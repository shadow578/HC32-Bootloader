#include "dwin.h"
#include "../../serial.h"

extern Serial screenSerial;

namespace dwin
{
  constexpr uint8_t DWIN_HEAD[] = { 0xAA };
  constexpr uint8_t DWIN_TAIL[] = { 0xCC, 0x33, 0xC3, 0x3C };

  /**
   * @brief send data to the DWIN screen
   * @param data the data to send
   * @param len the length of the data to send
   */
  void send(const uint8_t *data, const uint16_t len)
  {
    // send head
    for (uint8_t i = 0; i < sizeof(DWIN_HEAD); i++)
    {
      screenSerial.put(DWIN_HEAD[i]);
    }

    // send data
    for (uint16_t i = 0; i < len; i++)
    {
      screenSerial.put(data[i]);
    }

    // send tail
    for (uint8_t i = 0; i < sizeof(DWIN_TAIL); i++)
    {
      screenSerial.put(DWIN_TAIL[i]);
    }
  }

  /**
   * @brief send data to the DWIN screen
  */
  #define sendc(...)                       \
    {                                             \
      const uint8_t data[] = { __VA_ARGS__ };           \
      send(data, sizeof(data));                   \
    }

  /**
   * @brief 16 bit word to 2 bytes
   */
  #define DWIN_WORD(x) static_cast<uint8_t>((x >> 8) & 0xFF), static_cast<uint8_t>(x & 0xFF)

  void init()
  {
    // initialize the screen serial at 115200 baud
    screenSerial.init(115200);

    // send handshake, don't care about response
    sendc(0x00);
    redraw();
  }

  void fill_screen(const color::color color)
  {
    sendc(
      0x01, // cmd: clear screen
      DWIN_WORD(color),
    );
  }

  void redraw()
  {
    sendc(0x3D); // cmd: update screen

    //TODO: have to wait for the screen to finish ?
  }

  void set_brightness(const uint8_t brightness)
  {
    sendc(
      0x30, // cmd: set brightness
      brightness
    );
  }

  void set_orientation(const screen_orientation orientation)
  {
    sendc(
      0x34, // cmd: set orientation
      0x5A, // ?
      0xA5, // ?
      static_cast<uint8_t>(orientation)
    );
  }

  void draw_rectangle(
    const color::color color,
    const rectangle &rectangle,
    const bool fill)
  {
    sendc(
      0x05,                           // cmd: draw rectangle
      uint8_t(fill ? 1 : 0),          // fill mode
      DWIN_WORD(color),               // rectangle color
      DWIN_WORD(rectangle.x),         // x start (top left corner)
      DWIN_WORD(rectangle.y),         // y start
      DWIN_WORD(rectangle.x_end()),   // x end (bottom right corner)
      DWIN_WORD(rectangle.y_end()),   // y end
    );
  }

  size_t draw_string(
    const char *str,
    const uint16_t x,
    const uint16_t y,
    const font_size::font_size_t font,
    const color::color color,
    const color::color background,
    const bool draw_background)
  {
    // prepare command data
    const uint8_t cmd_data[] = {
      0x11,                           // cmd: draw string
      static_cast<uint8_t>(((draw_background ? 1 : 0) << 6) | (static_cast<uint8_t>(font) & 0x0F)), // flags: [7] = widthAdjust; [6] bShow; [5-4] = reserved; [3-0] = font size
      DWIN_WORD(color),               // text color
      DWIN_WORD(background),          // background color
      DWIN_WORD(x),                   // x start
      DWIN_WORD(y),                   // y start
      // string data
    };

    // calculate buffer size
    // limited to a total buffer size of 64 bytes
    const size_t str_len = min(strlen(str), 64 - sizeof(cmd_data));

    // create buffer and copy data over
    const size_t data_len = sizeof(cmd_data) + str_len;
    uint8_t data[data_len];
    std::copy(cmd_data, cmd_data + sizeof(cmd_data), data);
    std::copy(str, str + str_len, data + sizeof(cmd_data));

    // send data
    send(data, data_len);

    return str_len;
  }
  
  void move_screen_area(
    const rectangle &area,
    const screen_area_shift_dir dir,
    const uint16_t amount,
    const color::color fill_color)
  {
    sendc(
      0x09,                                           // cmd: move area
      uint8_t((1 << 7) | static_cast<uint8_t>(dir)),  // mode=translation; direction
      DWIN_WORD(amount),                              // distance
      DWIN_WORD(fill_color),                          // fill color
      DWIN_WORD(area.x),                              // x start (top left corner)
      DWIN_WORD(area.y),                              // y start
      DWIN_WORD(area.x_end()),                        // x end (bottom right corner)
      DWIN_WORD(area.y_end()),                        // y end
    );
  }
} // namespace dwin
