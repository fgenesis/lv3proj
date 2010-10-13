#include "GreedyTextField.h"

void gcn::GreedyTextField::keyPressed(KeyEvent& keyEvent)
{
    Key key = keyEvent.getKey();

    if (key.getValue() == Key::LEFT && mCaretPosition > 0)
    {
        --mCaretPosition;
    }

    else if (key.getValue() == Key::RIGHT && mCaretPosition < mText.size())
    {
        ++mCaretPosition;
    }

    else if (key.getValue() == Key::DELETE && mCaretPosition < mText.size())
    {
        mText.erase(mCaretPosition, 1);
    }

    else if (key.getValue() == Key::BACKSPACE && mCaretPosition > 0)
    {
        mText.erase(mCaretPosition - 1, 1);
        --mCaretPosition;
    }

    else if (key.getValue() == Key::ENTER)
    {
        distributeActionEvent();
        keyEvent.consume(); // <-- change
    }

    else if (key.getValue() == Key::HOME)
    {
        mCaretPosition = 0;
    }

    else if (key.getValue() == Key::END)
    {
        mCaretPosition = mText.size();
    }

    else if (key.isCharacter()
        && key.getValue() != Key::TAB)
    {
        mText.insert(mCaretPosition, std::string(1,(char)key.getValue()));
        ++mCaretPosition;
        keyEvent.consume(); // <-- this is the actual change, compared to a normal gcn::TextField
    }

    if (key.getValue() != Key::TAB)
    {
        keyEvent.consume();
    }

    fixScroll();
}
