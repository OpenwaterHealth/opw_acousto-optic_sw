#pragma once


#include <cstring>
#include <cstdint>

#define SFML_STATIC
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"



class CLI : public sf::Drawable {
public:

  CLI(int max_cmds);
  ~CLI();

  void Register(const char* cmd, const char* (*fn)(void*, const char*), void* param = NULL);
  void Input(char c);

  void Update();

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
  static const int BUFLEN = 80;
  struct mapper {
    char cmd[BUFLEN];
    const char* (*fn)(void*, const char*);
    void* param;
  };

  mapper* map_;
  int cmds_;

  char input_disp_[BUFLEN];
  char input_buf_[BUFLEN];
  int input_pos_ = 0;
  char output_buf_[BUFLEN];
  const char* output_ptr_;
  sf::Font font_;
  sf::Text input_text_;
  sf::Text output_text_;
};


