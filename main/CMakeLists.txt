set(SRCS
    "ShiftEV_CanTask.c"
    "can_drv/can_drv.c"
    "tim_drv/tim_drv.c"
)
idf_component_register(SRCS "${SRCS}"
                    INCLUDE_DIRS ".")
