/*
 * Copyright (C) 2014-2015-2015 Jolla Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authored by: Mohammed Hassan <mohammed.hassan@jolla.com>
 */

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/BinderService.h>
#include <gui/ISurfaceComposer.h>
#include <gui/IDisplayEventConnection.h>
#include <binder/IPermissionController.h>
#include <binder/MemoryHeapBase.h>
#include "allocator.h"

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 1 && ANDROID_MICRO == 2
#include "services/services_4_1_2.h"
#endif

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 2 && ANDROID_MICRO == 2
#include "services/services_4_2_2.h"
#endif

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4 && ANDROID_MICRO == 4
#include "services/services_4_4_4.h"
#endif

#if ANDROID_MAJOR == 5 && ANDROID_MINOR == 1
#include "services/services_5_1_0.h"
#endif

#if ANDROID_MAJOR == 6 && ANDROID_MINOR == 0
#include "services/services_6_0_0.h"
#endif

#if ANDROID_MAJOR == 7 && ANDROID_MINOR == 1
#include "services/services_7_1_0.h"
#endif

using namespace android;

#if (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4) || ANDROID_MAJOR >= 5
#include <binder/AppOpsManager.h>
#include <binder/IAppOpsService.h>
class FakeAppOps : public BinderService<FakeAppOps>,
           public BnAppOpsService
{
public:
  static char const *getServiceName() {
    return "appops";
  }

  virtual int32_t checkOperation(int32_t, int32_t, const String16&) {
    return android::AppOpsManager::MODE_ALLOWED;
  }

  virtual int32_t noteOperation(int32_t, int32_t, const String16&) {
    return android::AppOpsManager::MODE_ALLOWED;
  }

  virtual int32_t startOperation(const sp<IBinder>&, int32_t, int32_t,
                 const String16&) {
    return android::AppOpsManager::MODE_ALLOWED;
  }

  virtual void finishOperation(const sp<IBinder>&, int32_t, int32_t, const String16&) {
    // Nothing
  }

  virtual void startWatchingMode(int32_t, const String16&, const sp<IAppOpsCallback>&) {
    // Nothing
  }

  void stopWatchingMode(const sp<IAppOpsCallback>&) {
    // Nothing
  }

  virtual sp<IBinder> getToken(const sp<IBinder>&) {
    return NULL;
  }

#if ANDROID_MAJOR >= 6
  virtual int32_t permissionToOpCode(const String16& permission) {
    return 0;
  }
#endif
};

#endif

#if ANDROID_MAJOR >= 6

#include <binder/IProcessInfoService.h>

#if ANDROID_MAJOR >= 7

class BnProcessInfoService : public BnInterface<IProcessInfoService> {
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

status_t BnProcessInfoService::onTransact( uint32_t code, const Parcel& data, Parcel* reply,
        uint32_t flags) {
    switch(code) {
        case GET_PROCESS_STATES_FROM_PIDS: {
            CHECK_INTERFACE(IProcessInfoService, data, reply);
            int32_t arrayLen = data.readInt32();
            if (arrayLen <= 0) {
                reply->writeNoException();
                reply->writeInt32(0);
                reply->writeInt32(NOT_ENOUGH_DATA);
                return NO_ERROR;
            }

            size_t len = static_cast<size_t>(arrayLen);
            int32_t pids[len];
            status_t res = data.read(pids, len * sizeof(*pids));

            // Ignore output array length returned in the parcel here, as the states array must
            // always be the same length as the input PIDs array.
            int32_t states[len];
            for (size_t i = 0; i < len; i++) states[i] = -1;
            if (res == NO_ERROR) {
                res = getProcessStatesFromPids(len, /*in*/ pids, /*out*/ states);
            }
            reply->writeNoException();
            reply->writeInt32Array(len, states);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        case GET_PROCESS_STATES_AND_OOM_SCORES_FROM_PIDS: {
            CHECK_INTERFACE(IProcessInfoService, data, reply);
            int32_t arrayLen = data.readInt32();
            if (arrayLen <= 0) {
                reply->writeNoException();
                reply->writeInt32(0);
                reply->writeInt32(NOT_ENOUGH_DATA);
                return NO_ERROR;
            }

            size_t len = static_cast<size_t>(arrayLen);
            int32_t pids[len];
            status_t res = data.read(pids, len * sizeof(*pids));

            // Ignore output array length returned in the parcel here, as the
            // states array must always be the same length as the input PIDs array.
            int32_t states[len];
            int32_t scores[len];
            for (size_t i = 0; i < len; i++) {
                states[i] = -1;
                scores[i] = -10000;
            }
            if (res == NO_ERROR) {
                res = getProcessStatesAndOomScoresFromPids(
                        len, /*in*/ pids, /*out*/ states, /*out*/ scores);
            }
            reply->writeNoException();
            reply->writeInt32Array(len, states);
            reply->writeInt32Array(len, scores);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}
#endif


class FakeProcessInfoService : public BinderService<FakeProcessInfoService>,
                        public BnProcessInfoService
{
public:
    static char const *getServiceName() {
        return "processinfo";
    }

    status_t getProcessStatesFromPids(size_t length, int32_t* pids, int32_t* states) {
        for (int i=0; i< length; i++)
            states[i] = 0;
        return 0;
    }
    status_t getProcessStatesAndOomScoresFromPids( size_t length, int32_t* pids, int32_t* states, int32_t* scores) {
        for (int i=0; i< length; i++) {
            states[i] = 0;
            scores[i] = 0;
        }
        return 0;
    }
};

#include <binder/IBatteryStats.h>

class FakeBatteryStats : public BinderService<FakeBatteryStats>,
                                public BnBatteryStats
{
public:
    static char const *getServiceName() {
        return "batterystats";
    }
    void noteStartSensor(int uid, int sensor) {  }
    void noteStopSensor(int uid, int sensor) {  }
    void noteStartVideo(int uid) {  }
    void noteStopVideo(int uid) {  }
    void noteStartAudio(int uid) {  }
    void noteStopAudio(int uid) {  }
    void noteResetVideo() {  }
    void noteResetAudio() {  }
    void noteFlashlightOn(int uid) {  }
    void noteFlashlightOff(int uid) {  }
    void noteStartCamera(int uid) {  }
    void noteStopCamera(int uid) {  }
    void noteResetCamera() {  }
    void noteResetFlashlight() {  }
};


#include <gui/ISensorServer.h>
#include <gui/ISensorEventConnection.h>
#include <gui/Sensor.h>
#include <gui/BitTube.h>

class FakeSensorEventConnection : public BnSensorEventConnection
{
    sp<BitTube> mChannel;
public:
    FakeSensorEventConnection()
    {
        mChannel = new BitTube(0);
    }
    sp<BitTube> getSensorChannel() const {
        return mChannel;
    }
    status_t enableDisable(int handle, bool enabled, nsecs_t samplingPeriodNs,
                            nsecs_t maxBatchReportLatencyNs, int reservedFlags) {
        return 0;
    }
    status_t setEventRate(int handle, nsecs_t ns) {
        return 0;
    }
    status_t flush() {
        return 0;
    }
};
class FakeSensorServer : public BinderService<FakeSensorServer>,
                         public BnSensorServer
{
public:
    static char const *getServiceName() {
        return "sensorservice";
    }

    Vector<Sensor> getSensorList(const String16& opPackageName) {
        return Vector<Sensor>();
    }

#if ANDROID_MAJOR >= 7
    Vector<Sensor> getDynamicSensorList(const String16& opPackageName) {
        return Vector<Sensor>();
    }
#endif

    sp<ISensorEventConnection> createSensorEventConnection(const String8& packageName,
                        int mode, const String16& opPackageName) {
        return sp<ISensorEventConnection>(new FakeSensorEventConnection);
    }

    int32_t isDataInjectionEnabled() {
        return 0;
    }
};
#endif

class FakePermissionController : public BinderService<FakePermissionController>,
                                 public BnPermissionController
{
public:
    static char const *getServiceName() {
        return "permission";
    }

    bool checkPermission(const String16& permission, int32_t, int32_t) {
        if (permission == String16("android.permission.CAMERA")) {
            return true;
        }

        return false;
    }

#if ANDROID_MAJOR >= 6
    void getPackagesForUid(const uid_t uid, Vector<String16> &packages) {
    }

    bool isRuntimePermission(const String16& permission) {
         return false;
    }
#endif
};

int
main(int, char**)
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

    FakePermissionController::instantiate();
    MiniSurfaceFlinger::instantiate();

#if (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4) || ANDROID_MAJOR >= 5
    FakeAppOps::instantiate();
#endif
   
#if ANDROID_MAJOR >= 6
    FakeProcessInfoService::instantiate();
    FakeBatteryStats::instantiate();
    FakeSensorServer::instantiate();
#endif

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
