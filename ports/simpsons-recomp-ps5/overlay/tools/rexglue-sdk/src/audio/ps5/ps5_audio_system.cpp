#include <rex/assert.h>
#include <rex/audio/ps5/ps5_audio_driver.h>
#include <rex/audio/ps5/ps5_audio_system.h>

namespace rex::audio::ps5 {

PS5AudioSystem::PS5AudioSystem(runtime::FunctionDispatcher* function_dispatcher)
    : AudioSystem(function_dispatcher) {}

X_STATUS PS5AudioSystem::CreateDriver([[maybe_unused]] size_t index,
                                      rex::thread::Semaphore* semaphore,
                                      AudioDriver** out_driver) {
  assert_not_null(out_driver);
  auto* driver = new PS5AudioDriver(memory_, semaphore);
  if (!driver->Initialize()) {
    driver->Shutdown();
    delete driver;
    return X_STATUS_UNSUCCESSFUL;
  }
  *out_driver = driver;
  return X_STATUS_SUCCESS;
}

void PS5AudioSystem::DestroyDriver(AudioDriver* driver) {
  assert_not_null(driver);
  auto* ps5_driver = dynamic_cast<PS5AudioDriver*>(driver);
  assert_not_null(ps5_driver);
  ps5_driver->Shutdown();
  delete ps5_driver;
}

}  // namespace rex::audio::ps5
