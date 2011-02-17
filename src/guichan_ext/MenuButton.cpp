#include "guichan/font.hpp"
#include "guichan/exception.hpp"

#include "MenuButton.h"
#include "MenuBar.h"

namespace gcn {

MenuButton::MenuButton()
: Button(), _pressed(false)
{
    //setFrameSize(0);
    addActionListener(this);
    setAlignment(Graphics::LEFT);
}

MenuButton::MenuButton(const std::string& caption)
: Button(caption), _pressed(false)
{
    //setFrameSize(0);
    addActionListener(this);
    setAlignment(Graphics::LEFT);
}

MenuButton::~MenuButton()
{
}

void MenuButton::adjustSize()
{
    setWidth(15 + getFont()->getWidth(mCaption) + 2*mSpacing);
    setHeight(getFont()->getHeight() + 2*mSpacing);
}

void MenuButton::draw(Graphics* graphics)
{
    Color faceColor = getBaseColor();
    Color highlightColor, shadowColor;
    int alpha = getBaseColor().a;

    if (isPressed())
    {
        faceColor = faceColor - 0x303030;
        faceColor.a = alpha;
        highlightColor = faceColor - 0x303030;
        highlightColor.a = alpha;
        shadowColor = faceColor + 0x303030;
        shadowColor.a = alpha;
    }
    else
    {
        highlightColor = faceColor + 0x303030;
        highlightColor.a = alpha;
        shadowColor = faceColor - 0x303030;
        shadowColor.a = alpha;
    }

    graphics->setColor(faceColor);
    graphics->fillRectangle(Rectangle(0, 0, getDimension().width, getHeight()));

    graphics->setColor(getForegroundColor());

    int textX;
    int textY = getHeight() / 2 - getFont()->getHeight() / 2;

    switch (getAlignment())
    {
    case Graphics::LEFT:
        textX = mSpacing;
        break;
    case Graphics::CENTER:
        textX = getWidth() / 2;
        break;
    case Graphics::RIGHT:
        textX = getWidth() - mSpacing;
        break;
    default:
        throw GCN_EXCEPTION("Unknown alignment.");
    }

    graphics->setFont(getFont());

    if (isPressed())
    {
        graphics->drawText(getCaption(), textX + 1, textY + 1, getAlignment());
    }
    else
    {
        graphics->drawText(getCaption(), textX, textY, getAlignment());

        if (isFocused())
        {
            graphics->drawRectangle(Rectangle(2, 2, getWidth() - 4, getHeight() - 4));
        }
    }
}

void MenuButton::action(const ActionEvent& ae)
{
    _pressed = !_pressed;
}

bool MenuButton::isPressed()
{
    return _pressed;
}

void MenuButton::setPressed(bool b)
{
    _pressed = b;
}

void MenuButton::mouseEntered(MouseEvent& me)
{
    Button::mouseEntered(me);

    MenuBar *menu = (MenuBar*)getParent(); // <-- unsafe cast, beware!
    if(!menu)
        throw GCN_EXCEPTION("MenuButton has no parent menu");

    if(menu->getActiveButton())
    {
        menu->setActiveButton(this);
    }
}
        

} // end namespace gcn
