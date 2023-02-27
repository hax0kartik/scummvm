/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common/system.h"
#include "common/unicode-bidi.h"
#include "gui/widgets/edittext.h"
#include "gui/gui-manager.h"

#include "gui/ThemeEval.h"

namespace GUI {

EditTextWidget::EditTextWidget(GuiObject *boss, int x, int y, int w, int h, const Common::U32String &text, const Common::U32String &tooltip, uint32 cmd, uint32 finishCmd, ThemeEngine::FontStyle font)
	: EditableWidget(boss, x, y - 1, w, h + 2, tooltip, cmd) {
	setFlags(WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS | WIDGET_WANT_TICKLE);
	_type = kEditTextWidget;
	_finishCmd = finishCmd;

	_leftPadding = _rightPadding = 0;

	setEditString(text);
	setFontStyle(font);
}

EditTextWidget::EditTextWidget(GuiObject *boss, const Common::String &name, const Common::U32String &text, const Common::U32String &tooltip, uint32 cmd, uint32 finishCmd, ThemeEngine::FontStyle font)
	: EditableWidget(boss, name, tooltip, cmd) {
	setFlags(WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS | WIDGET_WANT_TICKLE);
	_type = kEditTextWidget;
	_finishCmd = finishCmd;

	_leftPadding = _rightPadding = 0;
	_shiftPressed = _isDragging = false;

	setEditString(text);
	setFontStyle(font);
}

void EditTextWidget::setEditString(const Common::U32String &str) {
	EditableWidget::setEditString(str);
	_backupString = str;
}

void EditTextWidget::reflowLayout() {
	_leftPadding = g_gui.xmlEval()->getVar("Globals.EditTextWidget.Padding.Left", 0);
	_rightPadding = g_gui.xmlEval()->getVar("Globals.EditTextWidget.Padding.Right", 0);

	EditableWidget::reflowLayout();
}

void EditTextWidget::drawWidget() {
	g_gui.theme()->drawWidgetBackground(Common::Rect(_x, _y, _x + _w, _y + _h),
	                                    ThemeEngine::kWidgetBackgroundEditText);

	// Draw the text
	adjustOffset();
	Common::Rect drawRect = getEditRect();
	drawRect.translate(_x, _y);
	setTextDrawableArea(drawRect);

	int x = drawRect.left;
	int y = drawRect.top;

	if (_align == Graphics::kTextAlignRight) {
		int strVisibleWidth = g_gui.getStringWidth(_editString, _font) - _editScrollOffset;
		if (strVisibleWidth > drawRect.width()) {
			_drawAlign = Graphics::kTextAlignLeft;
			strVisibleWidth = drawRect.width();
		} else {
			_drawAlign = _align;
		}
		x = drawRect.right - strVisibleWidth;
	}

	int selBegin = _selCaretPos;
	int selEnd = _selOffset + _selCaretPos;
	if (selBegin > selEnd)
		SWAP(selBegin, selEnd);
	
	if (_selOffset != 0) {
		Common::UnicodeBiDiText utxt(_editString);
		Common::U32String selectedString = Common::U32String(utxt.visual.c_str() + selBegin, selEnd - selBegin);
		int selBeginX = x + MIN(getSelectionCarretOffset(), getCaretOffset());
		int selEndX = selBeginX;

		for (uint i = 0, last = 0; i < selectedString.size(); ++i) {
			const uint cur = selectedString[i];
			selEndX += g_gui.getCharWidth(cur, _font) + g_gui.getKerningOffset(last, cur, _font);
			last = cur;
		}

		selBeginX = MAX(selBeginX, (int)drawRect.left);
		selEndX = MIN(selEndX, (int)drawRect.right);

		g_gui.theme()->drawText(drawRect, Common::Rect(selBeginX, y, selEndX, y + drawRect.height()), _editString, 
			                _state, _drawAlign, ThemeEngine::kTextInversionFocus, -_editScrollOffset , false, _font,
			                ThemeEngine::kFontColorNormal, true, _textDrawableArea);
	} else {
		g_gui.theme()->drawText(drawRect, _editString, _state, _drawAlign, 
			                ThemeEngine::kTextInversionNone, -_editScrollOffset , false, _font,
			                ThemeEngine::kFontColorNormal, true, _textDrawableArea);
	}
}

Common::Rect EditTextWidget::getEditRect() const {
	// Calculate (right - left) difference for editRect's X-axis coordinates:
	// (_w - 1 - _rightPadding) - (2 + _leftPadding)
	int editWidth = _w - _rightPadding - _leftPadding - 3;
	int editHeight = _h - 2;
	// Ensure r will always be a valid rect
	if (editWidth < 0) {
		editWidth = 0;
	}
	if (editHeight < 0) {
		editHeight = 0;
	}
	Common::Rect r(2 + _leftPadding, 1, 2 + _leftPadding + editWidth, 1 + editHeight);

	return r;
}

void EditTextWidget::receivedFocusWidget() {
	g_system->setFeatureState(OSystem::kFeatureVirtualKeyboard, true);
}

void EditTextWidget::lostFocusWidget() {
	// If we lose focus, 'commit' the user changes and clear selection
	_backupString = _editString;
	drawCaret(true);
	clearSelection();

	g_system->setFeatureState(OSystem::kFeatureVirtualKeyboard, false);
}

void EditTextWidget::startEditMode() {
}

void EditTextWidget::endEditMode() {
	releaseFocus();

	sendCommand(_finishCmd, 0);
}

void EditTextWidget::abortEditMode() {
	setEditString(_backupString);
	sendCommand(_cmd, 0);

	releaseFocus();
}

} // End of namespace GUI
