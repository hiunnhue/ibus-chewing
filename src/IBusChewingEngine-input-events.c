#include "IBusChewingProperties.h"

/**
 * ibus_chewing_engine_keycode_to_keysym:
 *
 * Convert keycode to keysym.
 */
guint ibus_chewing_engine_keycode_to_keysym(IBusChewingEngine * self,
					    guint keysym, guint keycode,
					    guint modifiers)
{
    guint kSym = keysym;
    if (!chewing_get_ChiEngMode(self->context)) {
	/* English mode, pass as-is */
	return kSym;
    }

    if (!ibus_chewing_engine_has_status_flag
	(self, ENGINE_FLAG_SYSTEM_KEYBOARD_LAYOUT)) {
	/* ibus_keymap_lookup_keysym treats keycode >= 256 */
	/* as IBUS_VoidSymbol */
	kSym = ibus_keymap_lookup_keysym(self->keymap_us, keycode,
					 modifiers);
	if (kSym == IBUS_VoidSymbol) {
	    /* Restore keysym */
	    kSym = keysym;
	}
    }

    return kSym;
}

gboolean ibus_chewing_engine_process_key_event(IBusEngine * engine,
					       guint keysym, guint keycode,
					       guint modifiers)
{
    gboolean result = TRUE;
    IBUS_CHEWING_LOG(MSG,
		     "* process_key_event(-,%x(%s),%x,%x) %s",
		     keysym, keyName_get(keysym), keycode, modifiers,
		     modifiers_to_string(modifiers));

    IBusChewingEngine *self = IBUS_CHEWING_ENGINE(engine);
    if (ibus_chewing_engine_is_password(self))
	return FALSE;
    guint kSym =
	ibus_chewing_engine_keycode_to_keysym(self, keysym, keycode,
					      modifiers);

    if (!(modifiers & IBUS_RELEASE_MASK)) {
	if (!keysym_KP_to_normal(kSym)
	    && (kSym == IBUS_Shift_L || kSym == IBUS_Shift_R)
	    && self->_priv->key_last == kSym) {
#if  !CHEWING_CHECK_VERSION(0,4,0)
	    /* When Chi->Eng with incomplete character */
	    if (chewing_get_ChiEngMode(self->context)
		&& !chewing_zuin_Check(self->context)) {
		/* chewing_zuin_Check==0 means incomplete character */
		/* Send a space to finish the character */
		chewing_handle_Space(self->context);
	    }
#endif
	    chewing_set_ChiEngMode(self->context,
				   !chewing_get_ChiEngMode(self->context));
	    self_refresh_property(self, "chewing_chieng_prop");
	    return self_update(self);
	}
	/* Skip press event */
	return TRUE;
    }

    IBUS_CHEWING_LOG(MSG,
		     "* process_key_event() kSym=%x(%s) keysym=%x(%s) ... continue",
		     kSym, keyName_get(kSym), keysym, keyName_get(keysym));
    guint state =
	modifiers & (IBUS_SHIFT_MASK | IBUS_CONTROL_MASK | IBUS_MOD1_MASK);
    self->_priv->key_last = kSym;
    if (state == 0) {
	guint kSym_tmp = keysym_KP_to_normal(kSym);
	if (kSym_tmp) {
	    IBUS_CHEWING_LOG(INFO,
			     "* process_key_event(): %x is from keypad.",
			     kSym_tmp);
	    /* Is keypad key */
	    if ((self->chewingFlags & CHEWING_FLAG_NUMPAD_ALWAYS_NUMBER)
		&& chewing_get_ChiEngMode(self->context)) {
		chewing_set_ChiEngMode(self->context, 0);
		self_handle_Default(self, kSym_tmp, FALSE);
		chewing_set_ChiEngMode(self->context, CHINESE_MODE);
	    } else {
		/* Convert kp numbers to normal */
		self_handle_Default(self, kSym_tmp, FALSE);
	    }
	} else {
	    switch (kSym) {
	    case IBUS_Return:
	    case IBUS_KP_Enter:
		ibus_chewing_engine_set_status_flag(self,
						    ENGINE_FLAG_NEED_COMMIT);
		chewing_handle_Enter(self->context);
		break;
	    case IBUS_Escape:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		chewing_handle_Esc(self->context);
		break;
	    case IBUS_BackSpace:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		chewing_handle_Backspace(self->context);
		break;
	    case IBUS_Delete:
	    case IBUS_KP_Delete:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		chewing_handle_Del(self->context);
		break;
	    case IBUS_space:
	    case IBUS_KP_Space:
		if (self->chewingFlags & CHEWING_FLAG_PLAIN_ZHUYIN) {
		    if (chewing_cand_TotalChoice(self->context) == 0) {
			chewing_handle_Space(self->context);
		    }
		} else {
			/**
			 * Fix for space in Temporary English mode.
			 */
		    chewing_set_easySymbolInput(self->context, 0);	/* fixed #33 first space wouldn't be committed */
		    chewing_handle_Space(self->context);
		    ibus_chewing_engine_set_status_flag(self,
							ENGINE_FLAG_NEED_COMMIT);
		}
		if (self->inputMode == CHEWING_INPUT_MODE_SELECTION_DONE ||
		    self->inputMode == CHEWING_INPUT_MODE_BYPASS ||
		    self->inputMode == CHEWING_INPUT_MODE_EDITING)
		    ibus_chewing_engine_set_status_flag(self,
							ENGINE_FLAG_NEED_COMMIT);
		break;
	    case IBUS_Page_Up:
	    case IBUS_KP_Page_Up:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		IBUS_ENGINE_GET_CLASS(engine)->page_up(engine);
		break;
	    case IBUS_Page_Down:
	    case IBUS_KP_Page_Down:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		IBUS_ENGINE_GET_CLASS(engine)->page_down(engine);
		break;
	    case IBUS_Up:
	    case IBUS_KP_Up:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		IBUS_ENGINE_GET_CLASS(engine)->cursor_up(engine);
		break;
	    case IBUS_Down:
	    case IBUS_KP_Down:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		IBUS_ENGINE_GET_CLASS(engine)->cursor_down(engine);
		break;
	    case IBUS_Left:
	    case IBUS_KP_Left:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		chewing_handle_Left(self->context);
		break;
	    case IBUS_Right:
	    case IBUS_KP_Right:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		chewing_handle_Right(self->context);
		break;
	    case IBUS_Home:
	    case IBUS_KP_Home:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		chewing_handle_Home(self->context);
		break;
	    case IBUS_End:
	    case IBUS_KP_End:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		chewing_handle_End(self->context);
		break;
	    case IBUS_Tab:
		if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		    return FALSE;
		chewing_handle_Tab(self->context);
		break;
	    case IBUS_Caps_Lock:
#if  !CHEWING_CHECK_VERSION(0,4,0)
		/* When Chi->Eng with incomplete character */
		if (chewing_get_ChiEngMode(self->context)
		    && !chewing_zuin_Check(self->context)) {
		    /* chewing_zuin_Check==0 means incomplete character */
		    /* Send a space to finish the character */
		    chewing_handle_Space(self->context);
		}
#endif
		chewing_handle_Capslock(self->context);
		self_refresh_property(self, "chewing_chieng_prop");
		break;
	    case IBUS_Shift_L:
	    case IBUS_Shift_R:
		/* Some QT application will sneak these through */
		return FALSE;
	    case IBUS_Alt_L:
	    case IBUS_Alt_R:
		/* Some QT application will sneak these through */
		return FALSE;
	    case IBUS_Control_L:
	    case IBUS_Control_R:
		/* Some QT application will sneak these through */
		return FALSE;
	    default:
		self_handle_Default(self, kSym, FALSE);
		break;
	    }
	}
    } else if (state == IBUS_SHIFT_MASK) {
	switch (kSym) {
	case IBUS_Return:
	case IBUS_KP_Enter:
	    /* Same with Shift Enter and Shift KP_Enter */
	    ibus_chewing_engine_set_status_flag(self,
						ENGINE_FLAG_NEED_COMMIT);
	    chewing_handle_Enter(self->context);
	    break;
	case IBUS_Left:
	    if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		return FALSE;
	    chewing_handle_ShiftLeft(self->context);
	    break;
	case IBUS_Right:
	    if (self->inputMode == CHEWING_INPUT_MODE_BYPASS)
		return FALSE;
	    chewing_handle_ShiftRight(self->context);
	    break;
	case IBUS_Up:
	case IBUS_KP_Up:
	case IBUS_Down:
	case IBUS_KP_Down:
	case IBUS_Page_Up:
	case IBUS_KP_Page_Up:
	case IBUS_Page_Down:
	case IBUS_KP_Page_Down:
	case IBUS_Home:
	case IBUS_End:
	    if (self->_priv->statusFlags & ENGINE_FLAG_NEED_COMMIT)
		self_force_commit(self);
	    return FALSE;
	case IBUS_space:
	case IBUS_KP_Space:
	    chewing_handle_ShiftSpace(self->context);
#if  !CHEWING_CHECK_VERSION(0,4,0)
	    chewing_set_ShapeMode(self->context,
				  !chewing_get_ShapeMode(self->context));
#endif
	    self_refresh_property(self, "chewing_alnumSize_prop");
	    break;
	default:
	    if (kSym > 127 || kSym < 0) {
		/* Special keys, must let it through */
		return FALSE;
	    }
	    self_handle_Default(self, kSym, TRUE);
	    break;
	}
    } else if (state == IBUS_CONTROL_MASK) {
	if (kSym >= IBUS_0 && kSym <= IBUS_9) {
	    chewing_handle_CtrlNum(self->context, kSym);
//      }else if (kSym==IBUS_v || kSym==IBUS_V){
//          chewing_handle_Right(self->context);
	} else {
	    result = FALSE;
	}

    } else {
	result = FALSE;
    }
    if (!result) {
	return FALSE;
    }
    return self_update(self);
}

void ibus_chewing_engine_handle_Default(IBusChewingEngine * self,
					guint keysym,
					gboolean shiftPressed)
{
    IBUS_CHEWING_LOG(DEBUG,
		     "handle_Default(-,%x,%s) plainZhuyin=%s inputMode=%d",
		     keysym,
		     (shiftPressed)? "TRUE": "FALSE",
		     (self->chewingFlags & CHEWING_FLAG_PLAIN_ZHUYIN) ?
		     "TRUE" : "FALSE", self->inputMode);
    ibus_chewing_engine_set_status_flag(self, ENGINE_FLAG_NEED_COMMIT);
    if (self->chewingFlags & CHEWING_FLAG_EASY_SYMBOL_INPUT) {
	/* If shift is pressed, turn on the  easySymbolInput, turn off
	 * otherwise
	 */
	chewing_set_easySymbolInput(self->context, (shiftPressed) ? 1 : 0);
    }
    if (self->chewingFlags & CHEWING_FLAG_FORCE_LOWERCASE_ENGLISH) {
	if (isupper(keysym) && !shiftPressed) {
	    keysym = tolower(keysym);
	} else if (islower(keysym) && shiftPressed) {
	    keysym = toupper(keysym);
	}
    }
    chewing_handle_Default(self->context, keysym);
    if (self->chewingFlags & CHEWING_FLAG_PLAIN_ZHUYIN) {
	if (self_is_selectKey(self, self->_priv->key_last) &&
	    self->inputMode == CHEWING_INPUT_MODE_SELECTING) {
	    chewing_handle_Enter(self->context);
	    self->inputMode = CHEWING_INPUT_MODE_SELECTION_DONE;
	}
    }
}

/*===================================================
 * Mouse events
 */
void ibus_chewing_engine_candidate_clicked(IBusEngine * engine,
					   guint index, guint button,
					   guint state)
{
    IBUS_CHEWING_LOG(INFO,
		     "*** candidate_clicked(-, %u, %u, %u) ... proceed.",
		     index, button, state);
    IBusChewingEngine *self = IBUS_CHEWING_ENGINE(engine);
    if (ibus_chewing_engine_is_password(self))
	return;
    if (index >= chewing_get_candPerPage(self->context) || index < 0) {
	IBUS_CHEWING_LOG(DEBUG, "candidate_clicked() index out of ranged");
	return;
    }
    if (self->inputMode == CHEWING_INPUT_MODE_SELECTING) {
	self->_priv->key_last = (guint) self->selKeys[index];
	ibus_chewing_engine_handle_Default(self, self->_priv->key_last,
					   FALSE);
	self_update(self);
    } else {
	IBUS_CHEWING_LOG(DEBUG, "candidate_clicked() ... Wrong mode: %u",
			 self->inputMode);
    }
}

void ibus_chewing_engine_property_activate(IBusEngine * engine,
					   const gchar * prop_name,
					   guint prop_state)
{
    IBUS_CHEWING_LOG(INFO, "property_activate(-, %s, %u)", prop_name,
		     prop_state);
    Self *self = SELF(engine);
    gboolean needRefresh = TRUE;
    if (STRING_EQUALS(prop_name, "chewing_chieng_prop")) {
	/* Toggle Chinese <-> English */
	chewing_set_ChiEngMode(self->context,
			       !chewing_get_ChiEngMode(self->context));
	gint isChinese = chewing_get_ChiEngMode(self->context);
	gint isCapsLockOff = (is_caps_led_on(self->_priv->pDisplay))
	    ? 0 : 1;
	IBUS_CHEWING_LOG(INFO,
			 "property_activate isChinese=%d isCapsLockOff=%d",
			 isChinese, isCapsLockOff);
	if (self->syncCapsLock_local != CHEWING_MODIFIER_SYNC_DISABLE) {
	    if (isChinese != isCapsLockOff) {
		set_caps_led(!isChinese, self->_priv->pDisplay);
		IBUS_CHEWING_LOG(DEBUG,
				 "property_activate isChinese=%d isCapsLockOff=%d",
				 isChinese, isCapsLockOff);
	    }
	}
    } else if (STRING_EQUALS(prop_name, "chewing_alnumSize_prop")) {
	/* Toggle Full <-> Half */
	chewing_set_ShapeMode(self->context,
			      !chewing_get_ShapeMode(self->context));
    } else if (STRING_EQUALS(prop_name, "chewing_settings_prop")) {
	if (ibus_chewing_property_get_state(self->settings_prop) ==
	    PROP_STATE_UNCHECKED) {
	    if (!self->sDialog) {
		MakerDialog *mDialog = maker_dialog_new_full
		    (self->iProperties->properties, _("Setting"),
		     MKDG_WIDGET_FLAG_APPLY_IMMEDIATELY,
		     MKDG_BUTTON_FLAG_OK | MKDG_BUTTON_FLAG_CANCEL);
		self->sDialog = GTK_WIDGET(mDialog);
	    }
	    gtk_widget_show_all(self->sDialog);
	    if (gtk_dialog_run(GTK_DIALOG(self->sDialog))
		== MKDG_BUTTON_RESPONSE_OK) {
		maker_dialog_assign_all_widgets_values(MAKER_DIALOG
						       (self->sDialog),
						       NULL);
	    }
	    gtk_widget_hide(self->sDialog);
#if IBUS_CHECK_VERSION(1, 4, 0)
	    ibus_property_set_state(self->settings_prop,
				    PROP_STATE_UNCHECKED);
#else
	    self->settings_prop->state = PROP_STATE_UNCHECKED;
#endif
	}
    } else {
	IBUS_CHEWING_LOG(DEBUG,
			 "property_activate(-, %s, %u) not recognized",
			 prop_name, prop_state);
	needRefresh = FALSE;
    }
    if (needRefresh)
	self_refresh_property(self, prop_name);
}

#if IBUS_CHECK_VERSION(1, 5, 4)
void ibus_chewing_engine_set_content_type(IBusEngine * engine,
					  guint purpose, guint hints)
{
    IBUS_CHEWING_LOG(DEBUG, "ibus_chewing_set_content_type(%d, %d)",
		     purpose, hints);

    Self *self = SELF(engine);
    if (purpose == IBUS_INPUT_PURPOSE_PASSWORD ||
	purpose == IBUS_INPUT_PURPOSE_PIN) {
	ibus_chewing_engine_set_status_flag(self, ENGINE_FLAG_IS_PASSWORD);
    } else {
	ibus_chewing_engine_clear_status_flag(self,
					      ENGINE_FLAG_IS_PASSWORD);
    }
}
#endif
