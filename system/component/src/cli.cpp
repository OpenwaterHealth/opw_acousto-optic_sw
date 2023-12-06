#include "system/component/inc/cli.h"

static const char PREFIX[] = "> ";

CLI::CLI(int max_cmds) {
  map_ = new mapper[max_cmds];
  cmds_ = 0;
  memset(input_buf_, 0, BUFLEN);
  memset(output_buf_, 0, BUFLEN);
  input_pos_ = 0;

  font_.loadFromFile("c:/windows/fonts/arial.ttf");
  input_text_.setFont(font_);
  input_text_.setCharacterSize(24);
  input_text_.setFillColor(sf::Color::White);
  input_text_.setStyle(sf::Text::Bold);
  output_text_.setFont(font_);
  output_text_.setCharacterSize(15);
  output_text_.setFillColor(sf::Color::White);
  //output_text_.setStyle(sf::Text::Bold);
  output_text_.setPosition(0, 25);

  strcpy_s(input_disp_, BUFLEN, PREFIX);
  strcat_s(input_disp_, input_buf_);
  strcpy_s(output_buf_, BUFLEN, "");
  output_ptr_ = output_buf_;
  input_text_.setString(input_disp_);
  output_text_.setString(output_ptr_);
}

CLI::~CLI() { delete[] map_; }

void CLI::Register(const char* cmd, const char* (*fn)(void*, const char*),
                   void* param) {
  strcpy_s(map_[cmds_].cmd, BUFLEN, cmd);
  map_[cmds_].fn = fn;
  map_[cmds_].param = param;
  ++cmds_;
}

void CLI::Input(char c) {
  switch (c) {
    case 0x08: {
      if (input_pos_ > 0) {
        input_buf_[--input_pos_] = 0;
      }
    } break;

    case 0x0D: {
      bool found = false;
      for (int i = 0; i < cmds_; ++i) {
        size_t len = strlen(map_[i].cmd);
        if (strncmp(input_buf_, map_[i].cmd, len) == 0 &&
            (input_buf_[len] == '\0' || input_buf_[len] == ' ')) {
          found = true;
          output_ptr_ = map_[i].fn(map_[i].param, &(input_buf_[len + 1]));
          break;
        }
      }

      if (!found) {
        strcpy_s(output_buf_, BUFLEN, "Unrecognized command: ");
        strcat_s(output_buf_, BUFLEN, input_buf_);
        output_ptr_ = output_buf_;
      }

      memset(input_buf_, 0, BUFLEN);
      input_pos_ = 0;
    } break;

    default: {
      input_buf_[input_pos_++] = c;
    } break;
  }

  strcpy_s(input_disp_, BUFLEN, PREFIX);
  strcat_s(input_disp_, input_buf_);
}

void CLI::Update() {
  input_text_.setString(input_disp_);
  output_text_.setString(output_ptr_);
}

void CLI::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  target.draw(input_text_, states);
  target.draw(output_text_, states);
}
