#pragma once

#include "core/factory_data.h"
#include "defines.h"
#include "proto.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern ProtoCtx protoCtx;
extern FactoryData factoryData;

void errorHandler();

#ifdef __cplusplus
}
#endif /* __cplusplus */