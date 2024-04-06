#include <hc32_ddl.h>
#include <cstdio>
#include "modules.h"

void on_progress(const flash::update_stage stage, const int done, const int total)
{
  logging::info(stage == flash::update_stage::erase ? "erase" : "write");
  logging::info(": ");
  logging::info(done, 10);
  logging::info(" of ");
  logging::info(total, 10);
  logging::info("\n");
  screen.showProgress((done * 100) / total);
  screen.flush();

  beep::beep(10, 1);
}

int main()
{
  // initialize system
  fault_handler::init();
  sysclock::apply();
  
  // initialize serial and ui
  #if HAS_SERIAL(HOST_SERIAL) 
    hostSerial.init(HOST_SERIAL_BAUD);
  #endif

  screen.init();

  #if ENABLE_BOOTLOADER_PROTECTION == 1
    // initialize protection of the bootloader region
    mpu::init(true, true, false);
    mpu::enable_region(0, mpu::region::build<
      LD_FLASH_START, 
      mpu::region::get_size(APP_BASE_ADDRESS - LD_FLASH_START), 
      mpu::permissions::get(mpu::permissions::READ_ONLY, mpu::permissions::READ_ONLY), 
      true>());
  #endif

  // print hello message
  logging::log("OpenHC32Boot " BOOTLOADER_VERSION "\n");
  beep::beep();

  #if PRINT_CPUID == 1
    cpuid::print();
  #endif

  // get the firmware file
  logging::log("checking ");
  logging::log(FIRMWARE_UPDATE_FILE);
  logging::log("\n");
  
  FIL file;
  flash::update_metadata metadata;
  if (sd::get_update_file(file, metadata, FIRMWARE_UPDATE_FILE))
  {
    // check if we've already flashed this firmware
    if (metadata.matches_stored())
    {
      logging::log("update skipped\n");
    }
    else
    {
      // apply the update
      if(!flash::apply_firmware_update(file, APP_BASE_ADDRESS, metadata, &on_progress))
      {
        logging::error("update failed\n");
        beep::beep(500, 999);
        ASSERT(false, "update failed");
      }
      else
      {
        logging::log("update applied\n");
      }
    }

    // close the file
    sd::close_update_file(file, FIRMWARE_UPDATE_FILE);
  }

  // log application jump address
  logging::log("jumping to app @ ");
  logging::log(APP_BASE_ADDRESS, 16);
  logging::log("\n");

  // run pre-checks on the application
  if (!leap::pre_check(APP_BASE_ADDRESS))
  {
    logging::log("pre-check fail! skip jump\n");
    beep::beep(250, 999);
    ASSERT(false, "pre-check fail");
  }

  // deinitialize serial to prevent interference with the application
  #if HAS_SERIAL(HOST_SERIAL)
    hostSerial.deinit();
  #endif
  #if HAS_SERIAL(SCREEN_SERIAL)
    screenSerial.deinit();
  #endif

  // restore the clock configuration
  sysclock::restore();

  // jump to the application
  leap::jump(APP_BASE_ADDRESS);
}
