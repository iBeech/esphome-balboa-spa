import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select

from .. import (
    balboa_spa_ns,
    BalboaSpa,
    CONF_SPA_ID,
)

DEPENDENCIES = ["balboa_spa"]
AUTO_LOAD = ["select"]

TemperatureRangeSelect = balboa_spa_ns.class_(
    "TemperatureRangeSelect", select.Select, cg.Component
)

CONF_TEMPERATURE_RANGE = "temperature_range"

TEMPERATURE_RANGE_OPTIONS = ["High", "Low"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_SPA_ID): cv.use_id(BalboaSpa),
        cv.Optional(CONF_TEMPERATURE_RANGE): select.select_schema(
            TemperatureRangeSelect,
            icon="mdi:thermometer-lines",
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SPA_ID])

    if conf := config.get(CONF_TEMPERATURE_RANGE):
        var = await select.new_select(conf, options=TEMPERATURE_RANGE_OPTIONS)
        await cg.register_component(var, conf)
        cg.add(var.set_parent(parent))
