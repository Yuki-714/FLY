/**
 * @file SimpleStandEngine.h
 *
 * A deliberately small motion module for learning the first part of a walking
 * engine: entering a statically stable stand from zero stiffness.
 */

#pragma once

#include "Framework/Module.h"
#include "Representations/Configuration/JointLimits.h"
#include "Representations/Configuration/RobotDimensions.h"
#include "Representations/Infrastructure/FrameInfo.h"
#include "Representations/Infrastructure/JointAngles.h"
#include "Representations/Infrastructure/JointRequest.h"
#include "Representations/Infrastructure/SensorData/KeyStates.h"
#include "Representations/Infrastructure/SensorData/RawInertialSensorData.h"

MODULE(SimpleStandEngine,
{,
  REQUIRES(FrameInfo),
  REQUIRES(JointAngles),
  REQUIRES(JointLimits),
  REQUIRES(KeyStates),
  REQUIRES(RawInertialSensorData),
  REQUIRES(RobotDimensions),
  PROVIDES(JointRequest),
  LOADS_PARAMETERS(
  {,
    (bool) autoStart, /**< Start automatically after relaxedDuration. Keep false on a physical robot. */
    (unsigned) relaxedDuration, /**< Unsupported zero-stiffness time before startup in ms. Use 0 in simulation. */
    (unsigned) stiffnessRampDuration, /**< Time to hold the measured pose before moving in ms. */
    (unsigned) standTransitionDuration, /**< Time to interpolate from the measured pose to standing in ms. */
    (float) standHeight, /**< Height of the hip above the sole plane in mm. */
    (float) torsoOffset, /**< Forward torso offset relative to the ankles in mm. */
    (float) extraFootSeparation, /**< Additional lateral distance of each foot from its hip in mm. */
    (Angle) maxStartTilt, /**< Maximum absolute torso roll/pitch from which standing may start. */
    (int) holdingStiffness, /**< Stiffness reached before changing the joint angles. */
    (int) standingLegStiffness, /**< Final stiffness of the leg joints. */
    (int) standingUpperBodyStiffness, /**< Final stiffness of head, waist, and arm joints. */
    (bool) balanceEnabled, /**< Enable IMU-based ankle balance after the standing transition. */
    (float) pitchKp, /**< Ankle pitch correction per radian of torso pitch. */
    (float) pitchKd, /**< Ankle pitch correction per radian/s of pitch velocity, in seconds. */
    (float) rollKp, /**< Ankle roll correction per radian of torso roll. */
    (float) rollKd, /**< Ankle roll correction per radian/s of roll velocity, in seconds. */
    (float) imuLowPassRatio, /**< Previous-sample weight of the IMU low-pass filter. */
    (Angle) maxAnkleCorrection, /**< Absolute limit for each ankle balance correction. */
    (bool) weightShiftEnabled, /**< Move the torso sideways while both feet remain fixed. */
    (float) weightShiftAmplitude, /**< Maximum lateral torso shift in mm. */
    (unsigned) weightShiftPhaseDuration, /**< Duration of each 0-to-side or side-to-0 phase in ms. */
    (Angle) standingShoulderPitch, /**< Shoulder pitch used for the standing arm pose. */
    (Angle) standingShoulderRoll, /**< Symmetric outward shoulder roll magnitude. */
    (Angle) standingElbowYaw, /**< Symmetric elbow yaw magnitude. */
    (Angle) standingElbowRoll, /**< Symmetric inward elbow roll magnitude. */
    (Angle) standingWristYaw, /**< Symmetric wrist yaw magnitude. */
  }),
});

class SimpleStandEngine : public SimpleStandEngineBase
{
  enum class State
  {
    relaxed,
    holdingCurrentPose,
    movingToStand,
    standing,
  };

  void update(JointRequest& jointRequest) override;

  void enterState(State newState);
  void captureCurrentPose();
  bool isUpright() const;
  bool calculateStandingPose();
  bool calculateStandingPose(float torsoShiftY, JointAngles& targetAngles) const;
  float calculateTorsoShiftY() const;
  void setStandingUpperBodyPose(JointAngles& targetAngles) const;
  void resetBalanceFilter();
  void applyAnkleBalance(JointRequest& jointRequest);
  void writeRelaxedRequest(JointRequest& jointRequest) const;
  void writeInterpolatedRequest(JointRequest& jointRequest, float ratio, int legStiffness, int upperBodyStiffness) const;
  static float smoothRatio(float ratio);

  State state = State::relaxed;
  unsigned stateStartTime = 0;
  bool initialized = false;
  bool autoStartConsumed = false;
  bool chestPressedLastFrame = false;
  bool standingPoseValid = false;
  float currentTorsoShiftY = 0.f;
  Vector2f filteredTorsoAngle = Vector2f::Zero();
  Vector2f filteredGyro = Vector2f::Zero();
  JointAngles startAngles;
  JointAngles standingAngles;
};
