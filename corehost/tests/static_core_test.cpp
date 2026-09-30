#include <corehost/static_core.hpp>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

using namespace corehost;

namespace mock {
lr::Environment env{}; lr::VideoRefresh video{}; lr::AudioSample audio{}; lr::AudioBatch batch{}; lr::InputPoll poll{}; lr::InputState input{};
bool init_called=false, load_called=false, unload_called=false, deinit_called=false;
void set_env(lr::Environment v){env=v;} void set_video(lr::VideoRefresh v){video=v;} void set_audio(lr::AudioSample v){audio=v;} void set_batch(lr::AudioBatch v){batch=v;} void set_poll(lr::InputPoll v){poll=v;} void set_input(lr::InputState v){input=v;}
void init(){
  init_called=true;
  lr::Variable vars[]={{"mock_speed","Speed; normal|fast"},{nullptr,nullptr}};
  assert(env(lr::env_set_variables,vars));
  lr::LogCallback logging{};
  assert(env(lr::env_get_log_interface,&logging));
  logging.log(lr::LogLevel::info,"mock core %s\n","initialized");
}
void deinit(){deinit_called=true;}
bool load(const lr::GameInfo* info){load_called=info&&info->path&&std::strlen(info->path)>0; return load_called;}
void unload(){unload_called=true;}
void av(lr::SystemAvInfo* out){ assert(out); out->geometry={320,240,640,480,4.0f/3.0f}; out->timing={60.0,44100.0}; }
void run(){poll(); static std::uint32_t pix[4]={}; video(pix,2,2,8); std::int16_t samples[4]={1,1,2,2}; batch(samples,2);}
void reset(){}
std::size_t state_size(){return 4;}
bool save(void* p,std::size_t n){if(n!=4)return false; std::memcpy(p,"TEST",4); return true;}
bool load_state(const void* p,std::size_t n){return n==4&&std::memcmp(p,"TEST",4)==0;}
}

int main(){
  lr::StaticApi api{mock::set_env,mock::set_video,mock::set_audio,mock::set_batch,mock::set_poll,mock::set_input,mock::init,mock::deinit,mock::load,mock::unload,mock::av,mock::run,mock::reset,mock::state_size,mock::save,mock::load_state};
  int frames=0; int audio_frames=0; int logs=0;
  Hooks hooks{};
  hooks.video=[&](const void*,unsigned w,unsigned h,std::size_t,lr::PixelFormat){ assert(w==2&&h==2); ++frames; };
  hooks.audio_batch=[&](const std::int16_t*,std::size_t n){audio_frames+=static_cast<int>(n);return n;};
  hooks.log=[&](std::string_view msg){ assert(msg.find("initialized")!=std::string_view::npos); ++logs; };
  hooks.input=[](unsigned){
    InputState s{};
    s.joypad_mask=1u<<8;
    s.left_x=123;
    s.keyboard[32u >> 6u] |= (std::uint64_t{1} << (32u & 63u));
    s.mouse_x=7;
    s.mouse_y=-4;
    s.mouse_wheel_y=1;
    s.mouse_buttons=1u<<0;
    return s;
  };

  StaticCore core("mock",api,{"/system","/save"},hooks);
  std::string err; assert(core.initialize(err)); assert(mock::init_called); assert(logs==1);
  assert(core.av_info().geometry.base_width==320);
  assert(core.av_info().geometry.base_height==240);
  assert(core.av_info().timing.sample_rate==44100.0);

  lr::Variable var{"mock_speed",nullptr};
  assert(mock::env(lr::env_get_variable,&var)); assert(std::string(var.value)=="normal");
  bool dirty=true;
  assert(mock::env(lr::env_get_variable_update,&dirty)); assert(!dirty);
  core.set_option("mock_speed","fast");
  assert(mock::env(lr::env_get_variable_update,&dirty)); assert(dirty);
  assert(mock::env(lr::env_get_variable_update,&dirty)); assert(!dirty);
  assert(mock::env(lr::env_get_variable,&var)); assert(std::string(var.value)=="fast");

  const char* dir=nullptr; assert(mock::env(lr::env_get_system_directory,&dir)); assert(std::string(dir)=="/system");
  auto fmt=lr::PixelFormat::xrgb8888; assert(mock::env(lr::env_set_pixel_format,&fmt));

  assert(core.load_path("/roms/test.rom",err)); core.run_frame(); assert(frames==1&&audio_frames==2);
  assert(mock::input(0,lr::device_joypad,0,8)==1);
  assert(mock::input(0,lr::device_analog,lr::analog_left,lr::analog_x)==123);
  assert(mock::input(0,lr::device_keyboard,0,32)==1);
  assert(mock::input(0,lr::device_mouse,0,lr::mouse_x)==7);
  assert(mock::input(0,lr::device_mouse,0,lr::mouse_y)==-4);
  assert(mock::input(0,lr::device_mouse,0,lr::mouse_left)==1);
  assert(mock::input(0,lr::device_mouse,0,lr::mouse_wheel_up)==1);

  std::vector<std::uint8_t> state;
  assert(core.save_state(state)&&state.size()==4);
  assert(core.load_state(state.data(),state.size()));
  core.shutdown();
  assert(mock::unload_called&&mock::deinit_called);
  return 0;
}
