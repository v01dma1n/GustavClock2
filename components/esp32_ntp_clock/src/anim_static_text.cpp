#include "anim_static_text.h"
#include "anim_time.h"
#include "anim_utils.h"

StaticTextAnimation::StaticTextAnimation(std::string text, bool dotsWithPreviousChar)
    : _text(std::move(text)), _dotsWithPreviousChar(dotsWithPreviousChar),
      _startTime(0), _rendered(false) {}

void StaticTextAnimation::setup(IDisplayDriver* display) {
    IAnimation::setup(display);
    _startTime = app_millis();
    _rendered = false;
}

void StaticTextAnimation::update() {
    if (_rendered) return;

    std::string parsedText;
    std::vector<uint8_t> dotStates;
    parseTextAndDots(_text, _dotsWithPreviousChar, parsedText, dotStates);

    const int cells = _display->getDisplaySize();
    for (int i = 0; i < cells; ++i) {
        char c   = (i < static_cast<int>(parsedText.size())) ? parsedText[i] : ' ';
        bool dot = (i < static_cast<int>(dotStates.size())) && dotStates[i];
        setChar(i, c, dot);
    }
    _rendered = true;
}

bool StaticTextAnimation::isDone() {
    // The scene manager controls how long we stay; we are always "ready
    // to be replaced", never "finished on our own".
    return false;
}
