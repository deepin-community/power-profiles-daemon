/*
 * Copyright (c) 2020 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include "ppd-driver.h"
#include "ppd-enums.h"

/**
 * SECTION:ppd-driver
 * @Short_description: Profile Drivers
 * @Title: Profile Drivers
 *
 * Profile drivers are the implementation of the different profiles for
 * the whole system. A driver will need to implement support `power-saver`
 * and `balanced` at a minimum.
 *
 * All drivers should be derived from either #PpdDriverCpu or #PpdDriverPlatform
 *
 * When a driver implements the `performance` profile, it might set the
 * #PpdDriver:performance-degraded property if the profile isn't running to
 * its fullest performance for any reason, such as thermal limits being
 * reached, or because a part of the user's body is too close for safety,
 * for example.
 */

typedef struct
{
  char          *driver_name;
  PpdProfile     profiles;
  gboolean       selected;
  char          *performance_degraded;
} PpdDriverPrivate;

enum {
  PROP_0,
  PROP_DRIVER_NAME,
  PROP_PROFILES,
  PROP_PERFORMANCE_DEGRADED
};

enum {
  PROFILE_CHANGED,
  PROBE_REQUEST,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define PPD_DRIVER_GET_PRIVATE(o) (ppd_driver_get_instance_private (o))
G_DEFINE_TYPE_WITH_PRIVATE (PpdDriver, ppd_driver, G_TYPE_OBJECT)

static void
ppd_driver_set_property (GObject        *object,
                         guint           property_id,
                         const GValue   *value,
                         GParamSpec     *pspec)
{
  PpdDriver *driver = PPD_DRIVER (object);
  PpdDriverPrivate *priv = PPD_DRIVER_GET_PRIVATE (driver);

  g_return_if_fail (PPD_IS_DRIVER (object));

  switch (property_id) {
  case PROP_DRIVER_NAME:
    g_return_if_fail (priv->driver_name == NULL);
    priv->driver_name = g_value_dup_string (value);
    break;
  case PROP_PROFILES:
    priv->profiles = g_value_get_flags (value);
    break;
  case PROP_PERFORMANCE_DEGRADED:
    {
      const char *degraded = g_value_get_string (value);

      g_clear_pointer (&priv->performance_degraded, g_free);

      if (degraded != NULL && *degraded != '\0')
        priv->performance_degraded = g_strdup (degraded);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ppd_driver_get_property (GObject        *object,
                         guint           property_id,
                         GValue         *value,
                         GParamSpec     *pspec)
{
  PpdDriver *driver = PPD_DRIVER (object);
  PpdDriverPrivate *priv = PPD_DRIVER_GET_PRIVATE (driver);

  switch (property_id) {
  case PROP_DRIVER_NAME:
    g_value_set_string (value, priv->driver_name);
    break;
  case PROP_PROFILES:
    g_value_set_flags (value, priv->profiles);
    break;
  case PROP_PERFORMANCE_DEGRADED:
    g_value_set_string (value, priv->performance_degraded);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ppd_driver_finalize (GObject *object)
{
  PpdDriverPrivate *priv;

  priv = PPD_DRIVER_GET_PRIVATE (PPD_DRIVER (object));
  g_clear_pointer (&priv->driver_name, g_free);
  g_clear_pointer (&priv->performance_degraded, g_free);

  G_OBJECT_CLASS (ppd_driver_parent_class)->finalize (object);
}

static void
ppd_driver_class_init (PpdDriverClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = ppd_driver_finalize;
  object_class->get_property = ppd_driver_get_property;
  object_class->set_property = ppd_driver_set_property;

  /**
   * PpdDriver::profile-changed:
   * @profile: the updated #PpdProfile
   *
   * Emitted when the profile was changed from the outside, usually
   * by key combinations implemented in firmware.
   */
  signals[PROFILE_CHANGED] = g_signal_new ("profile-changed",
                                           G_TYPE_FROM_CLASS (klass),
                                           G_SIGNAL_RUN_LAST,
                                           0,
                                           NULL,
                                           NULL,
                                           g_cclosure_marshal_generic,
                                           G_TYPE_NONE,
                                           1,
                                           PPD_TYPE_PROFILE);


  /**
   * PpdDriver::probe-request
   *
   * The driver requested to be reprobed, because it became available.
   */
  signals[PROBE_REQUEST] = g_signal_new ("probe-request",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0,
                                         NULL,
                                         NULL,
                                         g_cclosure_marshal_generic,
                                         G_TYPE_NONE,
                                         0,
                                         G_TYPE_NONE);

  /**
   * PpdDriver::driver-name:
   *
   * A unique driver name, only used for debugging.
   */
  g_object_class_install_property (object_class, PROP_DRIVER_NAME,
                                   g_param_spec_string ("driver-name",
                                                       "Driver name",
                                                       "Profile driver name",
                                                       NULL,
                                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * PpdDriver::profiles:
   *
   * The bitmask of #PpdProfile<!-- -->s implemented by this driver.
   */
  g_object_class_install_property (object_class, PROP_PROFILES,
                                   g_param_spec_flags ("profiles",
                                                      "Profiles",
                                                      "Profiles implemented by this driver",
                                                      PPD_TYPE_PROFILE,
                                                      0,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  /**
   * PpdPlatformDriver:performance-degraded:
   *
   * If set to a non-%NULL value, the reason why the performance profile is unavailable.
   * The value must be one of the options listed in the D-Bus API reference.
   */
  g_object_class_install_property (object_class, PROP_PERFORMANCE_DEGRADED,
                                   g_param_spec_string ("performance-degraded",
                                                       "Performance Degraded",
                                                       "Why the performance profile is degraded, if set",
                                                       NULL,
                                                       G_PARAM_READWRITE));
}

static void
ppd_driver_init (PpdDriver *self)
{
}

PpdProbeResult
ppd_driver_probe (PpdDriver  *driver)
{
  g_return_val_if_fail (PPD_IS_DRIVER (driver), FALSE);

  if (!PPD_DRIVER_GET_CLASS (driver)->probe)
    return PPD_PROBE_RESULT_SUCCESS;

  return PPD_DRIVER_GET_CLASS (driver)->probe (driver);
}

gboolean
ppd_driver_activate_profile (PpdDriver                   *driver,
                             PpdProfile                   profile,
                             PpdProfileActivationReason   reason,
                             GError                     **error)
{
  g_return_val_if_fail (PPD_IS_DRIVER (driver), FALSE);
  g_return_val_if_fail (ppd_profile_has_single_flag (profile), FALSE);

  if (!PPD_DRIVER_GET_CLASS (driver)->activate_profile)
    return TRUE;

  return PPD_DRIVER_GET_CLASS (driver)->activate_profile (driver, profile, reason, error);
}

gboolean
ppd_driver_power_changed (PpdDriver              *driver,
                          PpdPowerChangedReason   reason,
                          GError                **error)
{
  g_return_val_if_fail (PPD_IS_DRIVER (driver), FALSE);

  if (!PPD_DRIVER_GET_CLASS (driver)->power_changed)
    return TRUE;

  return PPD_DRIVER_GET_CLASS (driver)->power_changed (driver, reason, error);
}

gboolean ppd_driver_battery_changed (PpdDriver              *driver,
                                     gdouble                val,
                                     GError               **error)
{
  g_return_val_if_fail (PPD_IS_DRIVER (driver), FALSE);

  if (!PPD_DRIVER_GET_CLASS (driver)->battery_changed)
    return TRUE;

  return PPD_DRIVER_GET_CLASS (driver)->battery_changed (driver, val, error);
}

gboolean
ppd_driver_prepare_to_sleep (PpdDriver  *driver,
                             gboolean    start,
                             GError    **error)
{
  g_return_val_if_fail (PPD_IS_DRIVER (driver), FALSE);

  if (!PPD_DRIVER_GET_CLASS (driver)->prepare_to_sleep)
    return TRUE;

  return PPD_DRIVER_GET_CLASS (driver)->prepare_to_sleep (driver, start, error);
}

const char *
ppd_driver_get_driver_name (PpdDriver *driver)
{
  PpdDriverPrivate *priv;

  g_return_val_if_fail (PPD_IS_DRIVER (driver), NULL);

  priv = PPD_DRIVER_GET_PRIVATE (driver);
  return priv->driver_name;
}

PpdProfile
ppd_driver_get_profiles (PpdDriver *driver)
{
  PpdDriverPrivate *priv;

  g_return_val_if_fail (PPD_IS_DRIVER (driver), PPD_PROFILE_BALANCED);

  priv = PPD_DRIVER_GET_PRIVATE (driver);
  return priv->profiles;
}

gboolean
ppd_driver_get_selected (PpdDriver *driver)
{
  PpdDriverPrivate *priv;

  g_return_val_if_fail (PPD_IS_DRIVER (driver), FALSE);

  priv = PPD_DRIVER_GET_PRIVATE (driver);
  return priv->selected;
}

const char *
ppd_driver_get_performance_degraded (PpdDriver *driver)
{
  PpdDriverPrivate *priv;

  g_return_val_if_fail (PPD_IS_DRIVER (driver), NULL);

  priv = PPD_DRIVER_GET_PRIVATE (driver);
  return priv->performance_degraded;
}

gboolean
ppd_driver_is_performance_degraded (PpdDriver *driver)
{
  PpdDriverPrivate *priv;

  g_return_val_if_fail (PPD_IS_DRIVER (driver), FALSE);

  priv = PPD_DRIVER_GET_PRIVATE (driver);

  return (priv->performance_degraded != NULL);
}

void
ppd_driver_emit_profile_changed (PpdDriver  *driver,
                                 PpdProfile  profile)
{
  g_return_if_fail (PPD_IS_DRIVER (driver));
  g_return_if_fail (ppd_profile_has_single_flag (profile));

  g_signal_emit_by_name (G_OBJECT (driver),
                         "profile-changed",
                         profile);
}

const char *
ppd_profile_activation_reason_to_str (PpdProfileActivationReason reason)
{
  switch (reason) {
  case PPD_PROFILE_ACTIVATION_REASON_INTERNAL:
    return "internal";
  case PPD_PROFILE_ACTIVATION_REASON_RESET:
    return "reset";
  case PPD_PROFILE_ACTIVATION_REASON_USER:
    return "user";
  case PPD_PROFILE_ACTIVATION_REASON_RESUME:
    return "resume";
  case PPD_PROFILE_ACTIVATION_REASON_PROGRAM_HOLD:
    return "program-hold";
  default:
    g_return_val_if_reached (NULL);
  }
}
