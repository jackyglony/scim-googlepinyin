#include <cassert>

#include "google_imengine.h"
#include "composing_view.h"
#include "decoding_info.h"

ComposingView::ComposingView(GooglePyInstance *pinyin,
                             DecodingInfo *dec_info)
    : m_status(SHOW_PINYIN), m_pinyin(pinyin), m_dec_info(dec_info)
{}

ComposingView::Status
ComposingView::get_status() const
{
    return m_status;
}

void
ComposingView::reset()
{
    m_status = SHOW_PINYIN;
}

void
ComposingView::set_visibility(bool visible)
{
    if (visible) {
        m_pinyin->show_preedit_string();
    } else {
        m_pinyin->hide_preedit_string();
    }
}

void
ComposingView::move_cursor(int offset)
{
    if (m_status == EDIT_PINYIN) {
        m_dec_info->move_cursor(offset);
    } else if (m_status == SHOW_STRING_LOWERCASE) {
        m_status = EDIT_PINYIN;
    }
    invalidate();
}

void
ComposingView::redraw()
{
    switch (m_status) {
    case EDIT_PINYIN:
    case SHOW_PINYIN:
        draw_for_pinyin();
        break;
    case SHOW_STRING_LOWERCASE:
        draw_for_english();
        break;
    default:
        assert(false && "unknown composing status");
    }
}

// see PinyinInstance::refresh_aux_string()
void
ComposingView::draw_for_pinyin()
{
    size_t cursor_pos = m_dec_info->get_cursor_pos_in_cmps_display();
    int cmps_pos = cursor_pos;
    wstring cmps_str = m_dec_info->get_composing_str_for_display();
    size_t active_cmps_len = m_dec_info->get_active_cmps_display_len();
    if (cursor_pos > active_cmps_len) cmps_pos = active_cmps_len;
    
    AttributeList attrs;
    wstring aux;
    wstring item;
    int caret_pos = -1;
    
    item = cmps_str.substr(0, cmps_pos);
    attrs.push_back(
        Attribute(aux.length(), item.length(),
                  SCIM_ATTR_DECORATE, SCIM_ATTR_DECORATE_NONE));
    aux += item;
    
    if (cursor_pos <= active_cmps_len) {
        if (m_status == EDIT_PINYIN) {
            caret_pos = aux.length();
        }
        item = cmps_str.substr(cmps_pos, active_cmps_len - cmps_pos);
        attrs.push_back(
            Attribute(aux.length(), item.length(),
                      SCIM_ATTR_DECORATE, SCIM_ATTR_DECORATE_REVERSE));
        aux += item;
    }
    
    if (cmps_str.length() > active_cmps_len) {
        int orig_pos = active_cmps_len;
        if (cursor_pos > active_cmps_len) {
            if (cursor_pos > cmps_str.length()) cursor_pos = cmps_str.length();
            item = cmps_str.substr(orig_pos, cursor_pos - orig_pos);
            attrs.push_back(Attribute(aux.length(), item.length()));
            aux += item;
            if (m_status == EDIT_PINYIN) {
                caret_pos = aux.length();
            }
            
            orig_pos = cursor_pos;
        }
        item = cmps_str.substr(orig_pos);
        attrs.push_back(Attribute(aux.length(), item.length(),
                                  SCIM_ATTR_DECORATE, SCIM_ATTR_DECORATE_NONE));
    }
    m_pinyin->refresh_preedit_string(aux, attrs);
    m_pinyin->refresh_preedit_caret(caret_pos);
}

void
ComposingView::draw_for_english()
{
    AttributeList attrs;
    wstring aux;
    wstring item;

    aux = m_dec_info->get_original_spl_str();
    attrs.push_back(Attribute(0, aux.length(),
                              SCIM_ATTR_DECORATE, SCIM_ATTR_DECORATE_HIGHLIGHT));
    m_pinyin->refresh_preedit_string(aux, attrs);
}

void
ComposingView::invalidate()
{
    redraw();
}

void
ComposingView::set_decoding_info(DecodingInfo *dec_info,
                                 ImeState::State ime_status)
{
    m_dec_info = dec_info;
    if (ime_status == ImeState::STATE_INPUT) {
        m_status = SHOW_PINYIN;
        m_dec_info->move_cursor_to_edge(false);
    } else {
        if (dec_info->get_fixed_len() != 0 ||
            m_status == EDIT_PINYIN) {
            m_status = EDIT_PINYIN;
        } else {
            m_status = SHOW_STRING_LOWERCASE;
        }
        m_dec_info->move_cursor(0);
    }
    invalidate();
}