#pragma once

#include <LeifESPBase.h>
#include <LeifHomieLib.h>

extern HomieDevice homie;

void LeifHomieSetupDefaults(bool bDebug=true);
void LeifEnableMQTT(bool bEnable);

HomieProperty * NewSubscription(const String & strTopic);
