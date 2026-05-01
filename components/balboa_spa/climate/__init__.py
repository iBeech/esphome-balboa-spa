import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.const import CONF_ID

from .. import (
    CONF_SPA_ID,
    balboa_spa_ns,
    BalboaSpa,
)

DEPENDENCIES = ["balboa_spa", "climate"]
AUTO_LOAD = ["climate"]

BalboaSpaThermostat = balboa_spa_ns.class_('BalboaSpaThermostat', cg.Component, climate.Climate)

CONF_LEGACY_HVAC_MODES = "legacy_hvac_modes"
CONF_DYNAMIC_TEMPERATURE_LIMITS = "dynamic_temperature_limits"
CONF_HIGH_RANGE_MIN = "high_range_min_temperature"
CONF_HIGH_RANGE_MAX = "high_range_max_temperature"
CONF_LOW_RANGE_MIN = "low_range_min_temperature"
CONF_LOW_RANGE_MAX = "low_range_max_temperature"

CONFIG_SCHEMA = (
    climate.climate_schema(BalboaSpaThermostat).extend(
    {
        cv.GenerateID(): cv.declare_id(BalboaSpaThermostat),
        cv.GenerateID(CONF_SPA_ID): cv.use_id(BalboaSpa),
        cv.Optional(CONF_LEGACY_HVAC_MODES, default=True): cv.boolean,
        cv.Optional(CONF_DYNAMIC_TEMPERATURE_LIMITS, default=False): cv.boolean,
        cv.Optional(CONF_HIGH_RANGE_MIN, default=26.0): cv.temperature,
        cv.Optional(CONF_HIGH_RANGE_MAX, default=40.0): cv.temperature,
        cv.Optional(CONF_LOW_RANGE_MIN, default=10.0): cv.temperature,
        cv.Optional(CONF_LOW_RANGE_MAX, default=26.0): cv.temperature,
    })
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await climate.register_climate(var, config)

    parent = await cg.get_variable(config[CONF_SPA_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_legacy_hvac_modes(config[CONF_LEGACY_HVAC_MODES]))
    cg.add(var.set_dynamic_temperature_limits(config[CONF_DYNAMIC_TEMPERATURE_LIMITS]))
    cg.add(var.set_high_range_min(config[CONF_HIGH_RANGE_MIN]))
    cg.add(var.set_high_range_max(config[CONF_HIGH_RANGE_MAX]))
    cg.add(var.set_low_range_min(config[CONF_LOW_RANGE_MIN]))
    cg.add(var.set_low_range_max(config[CONF_LOW_RANGE_MAX]))
