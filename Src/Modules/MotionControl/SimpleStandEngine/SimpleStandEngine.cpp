/**
 * @file SimpleStandEngine.cpp
 */

#include "SimpleStandEngine.h"

#include "Debugging/Annotation.h"
#include "Debugging/Debugging.h"
#include "Debugging/Plot.h"
#include "Framework/Settings.h"
#include "Math/BHMath.h"
#include "Math/Pose3f.h"
#include "Streaming/Global.h"
#include "Tools/Motion/InverseKinematic.h"

#include <algorithm>
#include <cmath>

MAKE_MODULE(SimpleStandEngine);

namespace
{
  bool isUsableAngle(const Angle angle)
  {
    return std::isfinite(static_cast<float>(angle)) &&
           angle != JointAngles::off && angle != JointAngles::ignore;
  }

  int interpolateStiffness(const int from, const int to, const float ratio)
  {
    return static_cast<int>(std::round(from + (to - from) * ratio));
  }
}

void SimpleStandEngine::update(JointRequest& jointRequest)
{
  DECLARE_PLOT("module:SimpleStandEngine:balance:torsoPitch");
  DECLARE_PLOT("module:SimpleStandEngine:balance:torsoRoll");
  DECLARE_PLOT("module:SimpleStandEngine:balance:anklePitchCorrection");
  DECLARE_PLOT("module:SimpleStandEngine:balance:ankleRollCorrection");
  DECLARE_PLOT("module:SimpleStandEngine:weightShift:torsoY");

  if(!initialized)
  {
    initialized = true;
    stateStartTime = theFrameInfo.time;
    captureCurrentPose();
  }

  const bool chestPressed = theKeyStates.pressed[KeyStates::chest];
  const bool chestPressedThisFrame = chestPressed && !chestPressedLastFrame;
  chestPressedLastFrame = chestPressed;

  // This first implementation is intentionally restricted to the NAO geometry.
  // Other robot types remain unstiff until their own parameters are validated.
  if(Global::getSettings().robotType != Settings::nao)
  {
    writeRelaxedRequest(jointRequest);
    return;
  }

  // A single chest click only starts the controller. It must not remove all
  // stiffness while the robot is standing, because that would cause a fall.
  if(chestPressedThisFrame && state == State::relaxed)
  {
    autoStartConsumed = true;
    if(isUpright())
    {
      captureCurrentPose();
      enterState(State::holdingCurrentPose);
    }
    else
      ANNOTATION("SimpleStandEngine", "Start rejected: support the robot upright first.");
  }

  if(state == State::relaxed && autoStart && !autoStartConsumed &&
     theFrameInfo.getTimeSince(stateStartTime) >= static_cast<int>(relaxedDuration))
  {
    autoStartConsumed = true;
    if(isUpright())
    {
      captureCurrentPose();
      enterState(State::holdingCurrentPose);
    }
    else
    {
      ANNOTATION("SimpleStandEngine", "Auto-start rejected because the torso is tilted.");
      OUTPUT_ERROR("SimpleStandEngine: auto-start rejected because the torso is tilted.");
    }
  }

  switch(state)
  {
    case State::relaxed:
      writeRelaxedRequest(jointRequest);
      break;

    case State::holdingCurrentPose:
    {
      const float ratio = stiffnessRampDuration == 0 ? 1.f :
                          std::clamp(static_cast<float>(theFrameInfo.getTimeSince(stateStartTime)) /
                                     static_cast<float>(stiffnessRampDuration), 0.f, 1.f);
      // The measured pose is held with useful stiffness from the very first
      // control frame. Ramping from zero here lets gravity win before the
      // controller can support the robot.
      writeInterpolatedRequest(jointRequest, 0.f, holdingStiffness, holdingStiffness);

      if(ratio >= 1.f)
      {
        if(calculateStandingPose())
          enterState(State::movingToStand);
        else
        {
          ANNOTATION("SimpleStandEngine", "Standing pose is unreachable. Returning to zero stiffness.");
          OUTPUT_ERROR("SimpleStandEngine: standing pose is unreachable.");
          enterState(State::relaxed);
        }
      }
      break;
    }

    case State::movingToStand:
    {
      if(!standingPoseValid)
      {
        enterState(State::relaxed);
        writeRelaxedRequest(jointRequest);
        break;
      }

      const float ratio = standTransitionDuration == 0 ? 1.f :
                          std::clamp(static_cast<float>(theFrameInfo.getTimeSince(stateStartTime)) /
                                     static_cast<float>(standTransitionDuration), 0.f, 1.f);
      const float smooth = smoothRatio(ratio);
      writeInterpolatedRequest(jointRequest, smooth,
                               interpolateStiffness(holdingStiffness, standingLegStiffness, smooth),
                               interpolateStiffness(holdingStiffness, standingUpperBodyStiffness, smooth));

      if(ratio >= 1.f)
        enterState(State::standing);
      break;
    }

    case State::standing:
    {
      currentTorsoShiftY = calculateTorsoShiftY();
      JointAngles shiftedStandingAngles;
      if(calculateStandingPose(currentTorsoShiftY, shiftedStandingAngles))
        standingAngles = shiftedStandingAngles;
      else
      {
        // The configured 5 mm shift should always be reachable. Retaining the
        // previous valid pose is safer than sending an invalid IK result.
        currentTorsoShiftY = 0.f;
        ANNOTATION("SimpleStandEngine", "Weight-shift IK is unreachable; keeping the previous pose.");
      }

      writeInterpolatedRequest(jointRequest, 1.f, standingLegStiffness, standingUpperBodyStiffness);
      if(balanceEnabled)
        applyAnkleBalance(jointRequest);
      PLOT("module:SimpleStandEngine:weightShift:torsoY", currentTorsoShiftY);
      break;
    }
  }

  jointRequest.timestamp = theFrameInfo.time;
}

void SimpleStandEngine::enterState(const State newState)
{
  state = newState;
  stateStartTime = theFrameInfo.time;
  if(newState == State::relaxed)
  {
    standingPoseValid = false;
    currentTorsoShiftY = 0.f;
    captureCurrentPose();
  }
  else if(newState == State::standing)
  {
    currentTorsoShiftY = 0.f;
    resetBalanceFilter();
  }
}

void SimpleStandEngine::captureCurrentPose()
{
  startAngles = theJointAngles;
  FOREACH_ENUM(Joints::Joint, joint)
  {
    if(!isUsableAngle(startAngles.angles[joint]))
      startAngles.angles[joint] = 0_deg;
  }

  // SimRobot already creates the NAO with ShoulderPitch at 90deg. However,
  // the first JointAngles sample can still contain its zero-initialized value.
  // Never turn that transient 0deg into a stiff arms-forward command.
  setStandingUpperBodyPose(startAngles);
  standingAngles = startAngles;
}

bool SimpleStandEngine::isUpright() const
{
  const float roll = static_cast<float>(theRawInertialSensorData.angle.x());
  const float pitch = static_cast<float>(theRawInertialSensorData.angle.y());
  const float limit = static_cast<float>(maxStartTilt);
  return std::isfinite(roll) && std::isfinite(pitch) &&
         std::abs(roll) <= limit && std::abs(pitch) <= limit;
}

bool SimpleStandEngine::calculateStandingPose()
{
  standingPoseValid = calculateStandingPose(0.f, standingAngles);
  return standingPoseValid;
}

bool SimpleStandEngine::calculateStandingPose(const float torsoShiftY, JointAngles& targetAngles) const
{
  targetAngles = startAngles;
  setStandingUpperBodyPose(targetAngles);

  const Pose3f leftFoot(-torsoOffset,
                        theRobotDimensions.yHipOffset + extraFootSeparation - torsoShiftY,
                        -standHeight);
  const Pose3f rightFoot(-torsoOffset,
                         -theRobotDimensions.yHipOffset - extraFootSeparation - torsoShiftY,
                         -standHeight);

  if(!InverseKinematic::calcLegJoints(leftFoot, rightFoot, Vector2f::Zero(),
                                      targetAngles, theRobotDimensions))
    return false;

  FOREACH_ENUM(Joints::Joint, joint)
  {
    if(!isUsableAngle(targetAngles.angles[joint]))
      return false;
    theJointLimits.limits[joint].clamp(targetAngles.angles[joint]);
  }

  return true;
}

void SimpleStandEngine::setStandingUpperBodyPose(JointAngles& targetAngles) const
{
  // ShoulderPitch = 0deg points the NAO arms forwards. A value close to 90deg
  // lets them hang down and avoids moving the center of mass unnecessarily
  // far in front of the feet.
  targetAngles.angles[Joints::lShoulderPitch] = standingShoulderPitch;
  targetAngles.angles[Joints::rShoulderPitch] = standingShoulderPitch;
  targetAngles.angles[Joints::lShoulderRoll] = standingShoulderRoll;
  targetAngles.angles[Joints::rShoulderRoll] = -standingShoulderRoll;
  targetAngles.angles[Joints::lElbowYaw] = -standingElbowYaw;
  targetAngles.angles[Joints::rElbowYaw] = standingElbowYaw;
  targetAngles.angles[Joints::lElbowRoll] = -standingElbowRoll;
  targetAngles.angles[Joints::rElbowRoll] = standingElbowRoll;
  targetAngles.angles[Joints::lWristYaw] = -standingWristYaw;
  targetAngles.angles[Joints::rWristYaw] = standingWristYaw;
  targetAngles.angles[Joints::lHand] = 0.f;
  targetAngles.angles[Joints::rHand] = 0.f;
}

float SimpleStandEngine::calculateTorsoShiftY() const
{
  if(!weightShiftEnabled || weightShiftAmplitude == 0.f || weightShiftPhaseDuration == 0)
    return 0.f;

  // Four phases form one cycle:
  // center -> left -> center -> right -> center.
  const unsigned elapsed = static_cast<unsigned>(std::max(0, theFrameInfo.getTimeSince(stateStartTime)));
  const unsigned phase = (elapsed / weightShiftPhaseDuration) % 4;
  const float rawRatio = static_cast<float>(elapsed % weightShiftPhaseDuration) /
                         static_cast<float>(weightShiftPhaseDuration);
  const float ratio = smoothRatio(rawRatio);

  switch(phase)
  {
    case 0:
      return weightShiftAmplitude * ratio;
    case 1:
      return weightShiftAmplitude * (1.f - ratio);
    case 2:
      return -weightShiftAmplitude * ratio;
    default:
      return -weightShiftAmplitude * (1.f - ratio);
  }
}

void SimpleStandEngine::resetBalanceFilter()
{
  filteredTorsoAngle.x() = static_cast<float>(theRawInertialSensorData.angle.x());
  filteredTorsoAngle.y() = static_cast<float>(theRawInertialSensorData.angle.y());
  filteredGyro.x() = static_cast<float>(theRawInertialSensorData.gyro.x());
  filteredGyro.y() = static_cast<float>(theRawInertialSensorData.gyro.y());
}

void SimpleStandEngine::applyAnkleBalance(JointRequest& jointRequest)
{
  const float keep = std::clamp(imuLowPassRatio, 0.f, 1.f);
  const float useNew = 1.f - keep;

  const Vector2f torsoAngle(static_cast<float>(theRawInertialSensorData.angle.x()),
                            static_cast<float>(theRawInertialSensorData.angle.y()));
  const Vector2f gyro(static_cast<float>(theRawInertialSensorData.gyro.x()),
                       static_cast<float>(theRawInertialSensorData.gyro.y()));
  filteredTorsoAngle = keep * filteredTorsoAngle + useNew * torsoAngle;
  filteredGyro = keep * filteredGyro + useNew * gyro;

  const float limit = std::abs(static_cast<float>(maxAnkleCorrection));
  const Angle pitchCorrection(std::clamp(pitchKp * filteredTorsoAngle.y() +
                                         pitchKd * filteredGyro.y(), -limit, limit));
  const Angle rollCorrection(std::clamp(rollKp * filteredTorsoAngle.x() +
                                        rollKd * filteredGyro.x(), -limit, limit));

  // Both feet receive the same correction because the joint axes use the
  // same sign convention for this sagittal/lateral standing adjustment.
  jointRequest.angles[Joints::lAnklePitch] += pitchCorrection;
  jointRequest.angles[Joints::rAnklePitch] += pitchCorrection;
  jointRequest.angles[Joints::lAnkleRoll] += rollCorrection;
  jointRequest.angles[Joints::rAnkleRoll] += rollCorrection;

  theJointLimits.limits[Joints::lAnklePitch].clamp(jointRequest.angles[Joints::lAnklePitch]);
  theJointLimits.limits[Joints::rAnklePitch].clamp(jointRequest.angles[Joints::rAnklePitch]);
  theJointLimits.limits[Joints::lAnkleRoll].clamp(jointRequest.angles[Joints::lAnkleRoll]);
  theJointLimits.limits[Joints::rAnkleRoll].clamp(jointRequest.angles[Joints::rAnkleRoll]);

  PLOT("module:SimpleStandEngine:balance:torsoPitch", Angle(filteredTorsoAngle.y()).toDegrees());
  PLOT("module:SimpleStandEngine:balance:torsoRoll", Angle(filteredTorsoAngle.x()).toDegrees());
  PLOT("module:SimpleStandEngine:balance:anklePitchCorrection", pitchCorrection.toDegrees());
  PLOT("module:SimpleStandEngine:balance:ankleRollCorrection", rollCorrection.toDegrees());
}

void SimpleStandEngine::writeRelaxedRequest(JointRequest& jointRequest) const
{
  jointRequest.angles = theJointAngles.angles;
  FOREACH_ENUM(Joints::Joint, joint)
  {
    if(!isUsableAngle(jointRequest.angles[joint]))
      jointRequest.angles[joint] = startAngles.angles[joint];
    jointRequest.stiffnessData.stiffnesses[joint] = 0;
  }
  jointRequest.timestamp = theFrameInfo.time;
}

void SimpleStandEngine::writeInterpolatedRequest(JointRequest& jointRequest, const float ratio,
                                                  const int legStiffness, const int upperBodyStiffness) const
{
  FOREACH_ENUM(Joints::Joint, joint)
  {
    const float start = startAngles.angles[joint];
    const float target = standingAngles.angles[joint];
    jointRequest.angles[joint] = start + (target - start) * ratio;
    theJointLimits.limits[joint].clamp(jointRequest.angles[joint]);
    jointRequest.stiffnessData.stiffnesses[joint] =
      joint >= Joints::firstLegJoint ? std::clamp(legStiffness, 0, 100)
                                     : std::clamp(upperBodyStiffness, 0, 100);
  }
}

float SimpleStandEngine::smoothRatio(const float ratio)
{
  const float clipped = std::clamp(ratio, 0.f, 1.f);
  return 0.5f - 0.5f * std::cos(clipped * pi);
}
