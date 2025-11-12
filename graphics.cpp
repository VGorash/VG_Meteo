#include "graphics.h"

Element::Element(Adafruit_GFX* gfx, coordinates_t coords) : m_gfx(gfx), m_coords(coords), m_dirty(false){}

coordinates_t Element::getCoordinates()
{
  return m_coords;
}

bounds_t Element::getBounds()
{
  return {m_coords.x, m_coords.y, m_coords.w + m_coords.x, m_coords.h + m_coords.y};
}

bool Element::isDirty()
{
  return m_dirty;
}

void Element::setDirty()
{
  m_dirty = true;
}

 void Element::clearDirty()
{
  m_dirty = false;
}



TextElement::TextElement(Adafruit_GFX* gfx, coordinates_t coords, const String& text, const GFXfont* font) : Element(gfx, coords), m_font(font), m_text(text)
{
  m_gfx->setFont(m_font);
    
  int16_t tbx, tby; uint16_t tbh, tbw;
  m_gfx->getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

  m_coords.h = max(tbh, m_coords.h);
  m_coords.w = max(tbw, m_coords.w);

  if(tbw < m_coords.w)
  {
    m_textX = m_coords.x + m_coords.w / 2 - tbw / 2;
  }
  else
  {
    m_textX = m_coords.x;
  }
  m_textY = coords.y + tbh;
}

TextElement::TextElement(Adafruit_GFX* gfx, coordinates_t coords, const char* text, const GFXfont* font) : TextElement(gfx, coords, String(text), font){}


void TextElement::show()
{
  m_gfx->setFont(m_font);
  m_gfx->setTextColor(m_textColor);
  m_gfx->fillRect(m_coords.x, m_coords.y, m_coords.w, m_coords.h, m_backColor);
  m_gfx->drawRect(m_coords.x, m_coords.y, m_coords.w, m_coords.h, m_boundColor);
  m_gfx->setCursor(m_textX, m_textY);
  m_gfx->print(m_text);
}

void TextElement::setText(const String& text)
{
  if(text == m_text)
  {
    return;
  }
    
  m_gfx->setFont(m_font);

  int16_t tbx, tby; uint16_t tbh, tbw;
  m_gfx->getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

  m_coords.h = max(tbh, m_coords.h);
  m_coords.w = max(tbw, m_coords.w);

  if(tbw < m_coords.w)
  {
    m_textX = m_coords.x + m_coords.w / 2 - tbw / 2;
  }
  else
  {
    m_textX = m_coords.x;
  }
  m_textY = m_coords.y + tbh;

  m_text = text;

  setDirty();
}


ImageElement::ImageElement(Adafruit_GFX* gfx, coordinates_t coords, const uint8_t* data): Element(gfx, coords), m_data(data){}

void ImageElement::show()
{
  m_gfx->drawRect(m_coords.x, m_coords.y, m_coords.w, m_coords.h, m_boundColor);
  m_gfx->drawBitmap(m_coords.x, m_coords.y, m_data, m_coords.w, m_coords.h, m_color, m_backColor);
}

void ImageElement::setImage(const uint8_t* data)
{
  if(m_data == data)
  {
    return;
  }
  m_data = data;
  setDirty();
}
