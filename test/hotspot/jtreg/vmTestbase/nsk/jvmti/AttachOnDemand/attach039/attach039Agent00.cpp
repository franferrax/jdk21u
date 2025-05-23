/*
 * Copyright (c) 2007, 2024, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <jvmti.h>
#include <aod.h>
#include <jvmti_aod.hpp>

extern "C" {

/*
 * Expected agent work scenario:
 *  - during initialization agent enables ThreadStart and ThreadEnd events and starts new
 *    thread using JVMTI RunAgentThread
 *  - agent receives ThreadStart event for started thread
    - agent receives ThreadEnd event for started thread and finishes work
 */

#define STARTED_THREAD_NAME "ThreadStartedByAgent"

static Options* options = nullptr;
static const char* agentName;

static jvmtiEvent testEvents[] = { JVMTI_EVENT_THREAD_START, JVMTI_EVENT_THREAD_END };
static const int testEventsNumber = 2;

volatile int threadStartReceived = 0;
volatile int threadWasExecuted = 0;

void JNICALL startedThreadFunction(jvmtiEnv* jvmti, JNIEnv* jni, void* arg) {
    char threadName[MAX_STRING_LENGTH];

    if (!nsk_jvmti_aod_getThreadName(jvmti, nullptr, threadName)) {
        nsk_jvmti_aod_disableEventsAndFinish(agentName, testEvents, testEventsNumber, 0, jvmti, jni);
        return;
    }

    NSK_DISPLAY2("%s: thread started from agent is running (thread name: '%s')\n", agentName, threadName);

    threadWasExecuted = 1;
}

int startNewThread(jvmtiEnv* jvmti, JNIEnv* jni) {
    jthread thread;

    thread = nsk_jvmti_aod_createThreadWithName(jni, STARTED_THREAD_NAME);
    if (!NSK_VERIFY(thread != nullptr))
        return NSK_FALSE;

    if (!NSK_JVMTI_VERIFY(jvmti->RunAgentThread(thread, startedThreadFunction, nullptr, JVMTI_THREAD_NORM_PRIORITY))) {
        return NSK_FALSE;
    }

    NSK_DISPLAY1("%s: new thread was started\n", agentName);

    return NSK_TRUE;
}

void JNICALL threadEndHandler(jvmtiEnv *jvmti,
        JNIEnv* jni,
        jthread thread) {
    char threadName[MAX_STRING_LENGTH];

    if (!nsk_jvmti_aod_getThreadName(jvmti, thread, threadName)) {
        nsk_jvmti_aod_disableEventsAndFinish(agentName, testEvents, testEventsNumber, 0, jvmti, jni);
        return;
    }

    NSK_DISPLAY2("%s: ThreadEnd event was received for thread '%s'\n", agentName, threadName);

    if (!strcmp(STARTED_THREAD_NAME, threadName)) {
        int success = 1;

        if (!threadStartReceived) {
            NSK_COMPLAIN1("%s: ThreadStart event wasn't received\n", agentName);
            success = 0;
        }

        if (!threadWasExecuted) {
            NSK_COMPLAIN1("%s: started thread haven't execute its code\n", agentName);
            success = 0;
        }

        nsk_jvmti_aod_disableEventsAndFinish(agentName, testEvents, testEventsNumber, success, jvmti, jni);
    }
}

void JNICALL threadStartHandler(jvmtiEnv *jvmti,
        JNIEnv* jni,
        jthread thread) {
    char threadName[MAX_STRING_LENGTH];

    if (!nsk_jvmti_aod_getThreadName(jvmti, thread, threadName)) {
        nsk_jvmti_aod_disableEventsAndFinish(agentName, testEvents, testEventsNumber, 0, jvmti, jni);
        return;
    }

    NSK_DISPLAY2("%s: ThreadStart event was received for thread '%s'\n", agentName, threadName);

    if (!strcmp(STARTED_THREAD_NAME, threadName)) {
        threadStartReceived = 1;
    }
}

#ifdef STATIC_BUILD
JNIEXPORT jint JNI_OnLoad_attach039Agent00(JavaVM *jvm, char *options, void *reserved) {
    return JNI_VERSION_1_8;
}
#endif

JNIEXPORT jint JNICALL
#ifdef STATIC_BUILD
Agent_OnAttach_attach039Agent00(JavaVM *vm, char *optionsString, void *reserved)
#else
Agent_OnAttach(JavaVM *vm, char *optionsString, void *reserved)
#endif
{
    jvmtiEventCallbacks eventCallbacks;
    jvmtiEnv* jvmti;
    JNIEnv* jni;

    options = (Options*) nsk_aod_createOptions(optionsString);
    if (!NSK_VERIFY(options != nullptr))
        return JNI_ERR;

    agentName = nsk_aod_getOptionValue(options, NSK_AOD_AGENT_NAME_OPTION);

    jni = (JNIEnv*) nsk_aod_createJNIEnv(vm);
    if (jni == nullptr)
        return JNI_ERR;

    jvmti = nsk_jvmti_createJVMTIEnv(vm, reserved);
    if (!NSK_VERIFY(jvmti != nullptr))
        return JNI_ERR;

    memset(&eventCallbacks,0, sizeof(eventCallbacks));
    eventCallbacks.ThreadEnd = threadEndHandler;
    eventCallbacks.ThreadStart = threadStartHandler;
    if (!NSK_JVMTI_VERIFY(jvmti->SetEventCallbacks(&eventCallbacks, sizeof(eventCallbacks)))) {
        return JNI_ERR;
    }

    if (!(nsk_jvmti_aod_enableEvents(jvmti, testEvents, testEventsNumber))) {
        return JNI_ERR;
    }

    NSK_DISPLAY1("%s: initialization was done\n", agentName);

    if (!NSK_VERIFY(nsk_aod_agentLoaded(jni, agentName)))
        return JNI_ERR;

    NSK_DISPLAY1("%s: starting new thread\n", agentName);

    if (!NSK_VERIFY(startNewThread(jvmti, jni)))
        return JNI_ERR;

    return JNI_OK;
}

}
