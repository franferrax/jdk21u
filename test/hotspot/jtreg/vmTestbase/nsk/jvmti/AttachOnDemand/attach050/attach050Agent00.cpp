/*
 * Copyright (c) 2008, 2024, Oracle and/or its affiliates. All rights reserved.
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

#define ON_UNLOAD_MARKER "attach050.on_unload"

extern "C" {

/*
 * This agent gets JVMTI environment and calls nsk_aod_agentLoaded and
 * nsk_aod_agentFinished from Agent_OnAttach. It also has
 * Agent_OnUnload function that prints marker string to the stdout,
 * providing a means for the test runner to check whether
 * Agent_OnUnload is called on agent unload during VM shutdown.
 */

#ifdef STATIC_BUILD
JNIEXPORT jint JNI_OnLoad_attach050Agent00(JavaVM *jvm, char *options, void *reserved) {
    return JNI_VERSION_1_8;
}
#endif

JNIEXPORT jint JNICALL
#ifdef STATIC_BUILD
Agent_OnAttach_attach050Agent00(JavaVM *vm, char *optionsString, void *reserved)
#else
Agent_OnAttach(JavaVM *vm, char *optionsString, void *reserved)
#endif
{
    jvmtiEnv* jvmti;
    JNIEnv* jni = nullptr;
    Options* options = nullptr;
    const char* agentName;

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

    NSK_DISPLAY1("%s: initialization was done\n", agentName);

    if (!NSK_VERIFY(nsk_aod_agentLoaded(jni, agentName)))
        return JNI_ERR;

    nsk_aod_agentFinished(jni, agentName, 1);

    return JNI_OK;
}


/* agent library shutdown */
JNIEXPORT void JNICALL
#ifdef STATIC_BUILD
Agent_OnUnload_attach050Agent00(JavaVM *jvm)
#else
Agent_OnUnload(JavaVM *jvm)
#endif
{
    fprintf(stdout, "%s%s", ON_UNLOAD_MARKER, "\n");
    fflush(stdout);
}

}
