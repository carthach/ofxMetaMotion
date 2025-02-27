//
//  metamotionController.cpp
//  Created by Mach1 on 01/28/21.
//
//  References can be found at https://mbientlab.com/cppdocs/latest/index.html
//

#include "metamotionController.h"
#include "ofMain.h"

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;
using std::chrono::time_point;

metamotionController::metamotionController() {

}

metamotionController::~metamotionController() {
    if (isConnected){
        disconnectDevice(board);
    }
}

//----------------------------------------------------- setup.
void metamotionController::setup() {
    //getDeviceIDs();
    bleInterface.setup();
    resetOrientation();
//    search();
}

bool metamotionController::search() {
    isSearching = true;
    if (!bleInterface.connected){
        isConnected = false;
        
        // Looking for a device with "metamotion" in its name
        metaMotionDeviceIndex = bleInterface.findMetaMotionDevice(); // store autofound index
        if (metaMotionDeviceIndex == -1){
            // didn't find any
            // but they are not MetaMotion search again
            // rescan?
        } else if (metaMotionDeviceIndex > -1) {
            // found the device!
            // connect to the first found device in case the above didnt work
            bleInterface.connect(metaMotionDeviceIndex);
            // setup meta motion
            MblMwBtleConnection btleConnection;
            btleConnection.context = this;
            btleConnection.write_gatt_char = write_gatt_char;
            btleConnection.read_gatt_char = read_gatt_char;
            btleConnection.enable_notifications = enable_char_notify;
            btleConnection.on_disconnect = on_disconnect;
            board = mbl_mw_metawearboard_create(&btleConnection);

            mbl_mw_metawearboard_initialize(board, this, [](void* context, MblMwMetaWearBoard* board, int32_t status) -> void {
                if (!status) {
                    printf("Error initializing board: %d\n", status);
                } else {
                    printf("Board initialized\n");
                }
                auto dev_info = mbl_mw_metawearboard_get_device_information(board);
                auto *wrapper = static_cast<metamotionController *>(context);
                
                while (!mbl_mw_metawearboard_is_initialized(board)){
                    // Wait for async initialization finishes
                }
                if (mbl_mw_metawearboard_is_initialized(board) == 1) {
                    std::cout << "firmware revision number = " << dev_info->firmware_revision << std::endl;
                    std::cout << "model = " << dev_info->model_number << std::endl;
                    std::cout << "model = " << mbl_mw_metawearboard_get_model(board) << std::endl;
                    std::cout << "model = " << mbl_mw_metawearboard_get_model_name(board) << std::endl;

                    wrapper->enable_fusion_sampling(wrapper->board);
                    wrapper->get_current_power_status(wrapper->board);
                    wrapper->get_battery_percentage(wrapper->board);
                    wrapper->get_ad_name(wrapper->board);
                    wrapper->isConnected = true;
                    wrapper->currentlyInitializing = false;
                }
            });
            
            isSearching = false;
            return true;
        }
        
        if (bleInterface.devices.size() < 1) { // if there are no found devices search again
            bleInterface.rescanDevices();
        }
    }
    isSearching = false;
    return false;
}

void metamotionController::update(){
    if(bleInterface.connected){ // when connected section
        if (bUseMagnoHeading){
            angle[0] = outputEuler[0];
        } else {
            angle[0] = outputEuler[3];
        }
        angle[1] = outputEuler[1];
        angle[2] = outputEuler[2];
        /* Debug
        std::cout << "Y: " << angle[0];
        std::cout << "P: " << angle[1];
        std::cout << "R: " << angle[2] << std::endl;
         */
    }
}

void metamotionController::disconnectDevice(MblMwMetaWearBoard* board) {
    if (isConnected){
        disable_led(board);
        mbl_mw_metawearboard_free(board);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    isConnected = false;
    isSearching = false;
//    bleInterface.exit();
}

void metamotionController::data_printer(void* context, const MblMwData* data) {
    // Print data as 2 digit hex values
    uint8_t* data_bytes = (uint8_t*) data->value;
    string bytes_str("[");
    char buffer[5];
    for (uint8_t i = 0; i < data->length; i++) {
        if (i) {
            bytes_str += ", ";
        }
        //sprintf(buffer, "0x%02x", data_bytes[i]);
        bytes_str += buffer;
    }
    bytes_str += "]";

    // Format time as YYYYMMDD-HH:MM:SS.LLL
    time_point<system_clock> then(milliseconds(data->epoch));
    auto time_c = system_clock::to_time_t(then);
    auto rem_ms= data->epoch % 1000;

    std::cout << "{timestamp: " << put_time(localtime(&time_c), "%Y%m%d-%T") << "." << rem_ms << ", "
        << "type_id: " << data->type_id << ", "
        << "bytes: " << bytes_str.c_str() << "}"
        << std::endl;
}

void metamotionController::configure_sensor_fusion(MblMwMetaWearBoard* board) {
    // set fusion mode to ndof (n degress of freedom)
    mbl_mw_sensor_fusion_set_mode(board, MBL_MW_SENSOR_FUSION_MODE_NDOF);
    // set acceleration range to +/-16G, note accelerometer is configured here
    mbl_mw_sensor_fusion_set_acc_range(board, MBL_MW_SENSOR_FUSION_ACC_RANGE_4G);
    // set gyro range to 2000 DPS
    mbl_mw_sensor_fusion_set_gyro_range(board, MBL_MW_SENSOR_FUSION_GYRO_RANGE_2000DPS);    
    // write changes to the board
    mbl_mw_sensor_fusion_write_config(board);
    
    // set the tx power as high as allowed
    if (mbl_mw_metawearboard_get_model(board) == MBL_MW_MODEL_METAMOTION_S){
        mbl_mw_settings_set_tx_power(board, 8);
    } else {
        mbl_mw_settings_set_tx_power(board, 4);
    }
}

void metamotionController::get_current_power_status(MblMwMetaWearBoard* board) {
    auto power_status = mbl_mw_settings_get_power_status_data_signal(board);
    mbl_mw_datasignal_subscribe(power_status, this, [](void* context, const MblMwData* data) -> void {
        auto *wrapper = static_cast<metamotionController *>(context);
        std::cout << "Power Status: " << data << std::endl;
    });
    
    auto charge_status = mbl_mw_settings_get_charge_status_data_signal(board);
    mbl_mw_datasignal_subscribe(charge_status, this, [](void* context, const MblMwData* data) -> void {
        auto *wrapper = static_cast<metamotionController *>(context);
        std::cout << "Charge Status: " << data << std::endl;
    });
}

void metamotionController::get_battery_percentage(MblMwMetaWearBoard* board) {
    auto battery_signal = mbl_mw_settings_get_battery_state_data_signal(board);
    mbl_mw_datasignal_subscribe(battery_signal, this, [](void* context, const MblMwData* data) -> void {
        auto *wrapper = static_cast<metamotionController *>(context);
        auto state = (MblMwBatteryState*) data->value;
        wrapper->battery_level = state->charge;
        //printf("{voltage: %dmV, charge: %d}\n", state->voltage, state->charge);
    });
    mbl_mw_datasignal_read(battery_signal);
}

void metamotionController::set_ad_name(MblMwMetaWearBoard* board) {
    const char* charName;
    if (mbl_mw_metawearboard_get_model(board) == MBL_MW_MODEL_METAMOTION_S){
        charName = "Mach1-MMS";
    } else if (mbl_mw_metawearboard_get_model(board) == MBL_MW_MODEL_METAMOTION_RL){
        charName = "Mach1-MMRL";
    } else if (mbl_mw_metawearboard_get_model(board) == MBL_MW_MODEL_METAWEAR_R){
        charName = "Mach1-MMR";
    } else {
        charName = "MetaWear";
    }
    size_t length = strlen(charName) + 1;
    const char* beg = charName;
    const char* end = charName + length;
    uint8_t* name = new uint8_t[length];
    size_t i = 0;
    for (; beg != end; ++beg, ++i){
        name[i] = (uint8_t)(*beg);
    }
    mbl_mw_settings_set_device_name(board, name, strlen(charName));
}

void metamotionController::get_ad_name(MblMwMetaWearBoard* board){
    // This function is for calling the name via metamotion
    module_name = mbl_mw_metawearboard_get_model_name(board);    
}

void metamotionController::enable_fusion_sampling(MblMwMetaWearBoard* board) {
    // Write the config to the sensor
    configure_sensor_fusion(board);
    
    auto fusion_signal = mbl_mw_sensor_fusion_get_data_signal(board, MBL_MW_SENSOR_FUSION_DATA_EULER_ANGLE);
    mbl_mw_datasignal_subscribe(fusion_signal, this, [](void* context, const MblMwData* data) -> void {
        auto *wrapper = static_cast<metamotionController *>(context);
        
        auto euler = (MblMwEulerAngles*)data->value;
        if (wrapper->bUseMagnoHeading){ // externally set use of magnometer to correct IMU or not
            wrapper->outputEuler[0] = euler->heading;
            wrapper->outputEuler[3] = euler->yaw;
        } else {
            wrapper->outputEuler[0] = euler->yaw;
            wrapper->outputEuler[3] = euler->heading;
        }
        wrapper->outputEuler[1] = euler->pitch;
        wrapper->outputEuler[2] = euler->roll;
        //printf("(%.3f, %.3f, %.3f)\n", euler->yaw, euler->pitch, euler->roll);
    });
    
    auto acceleration_signal = mbl_mw_sensor_fusion_get_data_signal(board, MBL_MW_SENSOR_FUSION_DATA_CORRECTED_ACC);
    mbl_mw_datasignal_subscribe(acceleration_signal, this, [](void* context, const MblMwData* data) -> void {
        auto *wrapper = static_cast<metamotionController *>(context);
        
        auto acceleration = (MblMwCorrectedCartesianFloat*) data->value;
        wrapper->outputAcceleration[0] = acceleration->x;
        wrapper->outputAcceleration[1] = acceleration->y;
        wrapper->outputAcceleration[2] = acceleration->z;
    });
    
    auto gyro_signal = mbl_mw_sensor_fusion_get_data_signal(board, MBL_MW_SENSOR_FUSION_DATA_CORRECTED_GYRO);
    mbl_mw_datasignal_subscribe(gyro_signal, this, [](void* context, const MblMwData* data) -> void {
        auto *wrapper = static_cast<metamotionController *>(context);
        
        auto gyro = (MblMwCorrectedCartesianFloat*) data->value;
        wrapper->outputGyro[0] = gyro->x;
        wrapper->outputGyro[1] = gyro->y;
        wrapper->outputGyro[2] = gyro->z;
    });
    
    auto mag_signal = mbl_mw_sensor_fusion_get_data_signal(board, MBL_MW_SENSOR_FUSION_DATA_CORRECTED_MAG);
    mbl_mw_datasignal_subscribe(mag_signal, this, [](void* context, const MblMwData* data) -> void {
        auto *wrapper = static_cast<metamotionController *>(context);
        
        auto mag = (MblMwCorrectedCartesianFloat*) data->value;
        wrapper->outputMag[0] = mag->x;
        wrapper->outputMag[1] = mag->y;
        wrapper->outputMag[2] = mag->z;
    });
    
    // Start
    
    mbl_mw_sensor_fusion_enable_data(board, MBL_MW_SENSOR_FUSION_DATA_EULER_ANGLE);
    mbl_mw_sensor_fusion_enable_data(board, MBL_MW_SENSOR_FUSION_DATA_CORRECTED_ACC);
    mbl_mw_sensor_fusion_enable_data(board, MBL_MW_SENSOR_FUSION_DATA_CORRECTED_GYRO);
    mbl_mw_sensor_fusion_enable_data(board, MBL_MW_SENSOR_FUSION_DATA_CORRECTED_MAG);
    
    mbl_mw_sensor_fusion_start(board);
    enable_led(board);
}

void metamotionController::enable_led(MblMwMetaWearBoard* board) {
    MblMwLedPattern pattern;
    pattern = { 8, 0, 250, 250, 250, 5000, 0, 0 };
    mbl_mw_led_write_pattern(board, &pattern, MBL_MW_LED_COLOR_RED);
    mbl_mw_led_play(board);
}

void metamotionController::disable_led(MblMwMetaWearBoard* board) {
    // stop the LED pattern from playing
    mbl_mw_led_stop_and_clear(board);
}

void metamotionController::disable_fusion_sampling(MblMwMetaWearBoard* board) {
    auto fusion_signal = mbl_mw_sensor_fusion_get_data_signal(board, MBL_MW_SENSOR_FUSION_DATA_EULER_ANGLE);
    mbl_mw_datasignal_unsubscribe(fusion_signal);
    mbl_mw_sensor_fusion_stop(board);
}

void metamotionController::calibration_mode(MblMwMetaWearBoard* board) {
    
}

void metamotionController::resetOrientation() {
    for(int i=0; i< 3; i++){
        angle_shift[i] = 0;
    }
}

void metamotionController::recenter() {
    float* swpAngle = getAngle();
    angle_shift[0] = angle_shift[0] - swpAngle[0];
    angle_shift[1] = angle_shift[1] - swpAngle[1];
    angle_shift[2] = angle_shift[2] - swpAngle[2];
}

float* metamotionController::getAngle() {
    float* calculated_angle = new float[3];
        
    calculated_angle[0] = angle[0] + angle_shift[0];
    calculated_angle[1] = angle[1] + angle_shift[1];
    calculated_angle[2] = angle[2] + angle_shift[2];
    
    return calculated_angle;
}

string HighLow2Uuid(const uint64_t high, const uint64_t low){
    uint8_t *u_h = (uint8_t *)&(high);
    uint8_t *u_l = (uint8_t *)&(low);

    char UUID[38];
    std::sprintf(UUID, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            u_h[7], u_h[6], u_h[5], u_h[4], u_h[3], u_h[2], u_h[1], u_h[0],
            u_l[7], u_l[6], u_l[5], u_l[4], u_l[3], u_l[2], u_l[1], u_l[0]
            );
    
    return string(UUID);
}

void metamotionController::read_gatt_char(void *context, const void *caller, const MblMwGattChar *characteristic,
                                          MblMwFnIntVoidPtrArray handler) {
    auto *wrapper = static_cast<metamotionController *>(context);
    if (wrapper->metaMotionDeviceIndex != -1) {
        auto readByteArray = wrapper->bleInterface.devices[wrapper->metaMotionDeviceIndex].read(HighLow2Uuid(characteristic->service_uuid_high, characteristic->service_uuid_low), HighLow2Uuid(characteristic->uuid_high, characteristic->uuid_low));
                                                     
        handler(caller, (uint8_t*)readByteArray.data(), readByteArray.length());
    }
}

void metamotionController::write_gatt_char(void *context, const void *caller, MblMwGattCharWriteType writeType,
                                          const MblMwGattChar *characteristic, const uint8_t *value, uint8_t length){
    auto *wrapper = static_cast<metamotionController *>(context);
    if (wrapper->metaMotionDeviceIndex != -1) {
        wrapper->bleInterface.devices[wrapper->metaMotionDeviceIndex].write_command(HighLow2Uuid(characteristic->service_uuid_high, characteristic->service_uuid_low), HighLow2Uuid(characteristic->uuid_high, characteristic->uuid_low), std::string((char*)value, int(length)));
    }
}

void metamotionController::enable_char_notify(void *context, const void *caller, const MblMwGattChar *characteristic,
                                             MblMwFnIntVoidPtrArray handler, MblMwFnVoidVoidPtrInt ready) {

    auto *wrapper = static_cast<metamotionController *>(context);
    if (wrapper->metaMotionDeviceIndex != -1) {
        wrapper->bleInterface.devices[wrapper->metaMotionDeviceIndex].notify(HighLow2Uuid(characteristic->service_uuid_high, characteristic->service_uuid_low), HighLow2Uuid(characteristic->uuid_high, characteristic->uuid_low), [&,handler,caller](SimpleBLE::ByteArray payload) {
            handler(caller,(uint8_t*)payload.data(),payload.length());
        });
        ready(caller, MBL_MW_STATUS_OK);
    }
}

void metamotionController::on_disconnect(void *context, const void *caller, MblMwFnVoidVoidPtrInt handler) {
    auto *wrapper = static_cast<metamotionController *>(context);
    handler(caller, MBL_MW_STATUS_OK);
}
