#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Adafruit_GFX.h>

typedef struct bounds
{
  uint16_t left;
  uint16_t top;
  uint16_t right;
  uint16_t bottom;
} bounds_t;

typedef struct coordinates
{
  uint16_t x;
  uint16_t y;
  uint16_t w;
  uint16_t h;
} coordinates_t;


class Element
{
public:
  Element(Adafruit_GFX* gfx, coordinates_t coords);

  coordinates_t getCoordinates();
  bounds_t getBounds();

  bool isDirty();
  void setDirty();
  void clearDirty();

  virtual void show() = 0;

protected:
  Adafruit_GFX* m_gfx;
  coordinates_t m_coords;
  bool m_dirty;
};

class TextElement : public Element
{
public:
  TextElement(Adafruit_GFX* gfx, coordinates_t coords, const String& text, const GFXfont* font);
  TextElement(Adafruit_GFX* gfx, coordinates_t coords, const char* text, const GFXfont* font);

  void show() override;

  void setText(const String& text);

protected:
  String m_text;
  const GFXfont* m_font;
  const uint16_t m_textColor = 0x0000;
  const uint16_t m_backColor = 0xFFFF;
  const uint16_t m_boundColor = 0XFFFF;

  uint16_t m_textX;
  uint16_t m_textY;
};

class ImageElement : public Element
{
public:
  ImageElement(Adafruit_GFX* gfx, coordinates_t coords, const uint8_t* data);

  void show() override;

  void setImage(const uint8_t* data);

private:
  const uint8_t* m_data;
  const uint16_t m_color = 0x0000;
  const uint16_t m_backColor = 0xFFFF;
  const uint16_t m_boundColor = 0XFFFF;
};

#endif