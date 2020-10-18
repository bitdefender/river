import os

from gym.envs.registration import register

register(
    id='RiverBinaryFuzzerBase-v0',
    entry_point='gymEnvs.envs:RiverBinaryFuzzerBase',
)
