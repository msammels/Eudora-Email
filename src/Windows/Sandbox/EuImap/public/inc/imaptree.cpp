// imaptree.cpp: implementation
//
// IMAP specific additions to CMboxTreeCtrl. This is #included at the bottom 
// of mboxtree.cpp.


#ifdef IMAP4	// Valid only if IMAP is enabled.

#include "imapactl.h"

// ======== Functions shared with mboxtree.cpp: ==============//
void AddImapMenus (	HMENU hMenu );

//=============================================//

////////////////////////////////////////////////////////////////////////
// ImapHandleDblclkFolder [protected]
//
// This is called when a an attempt is made to open an IMAP folder icon. We need to
// display any children BUT we may need to open this as a mailbox as well.
//  
////////////////////////////////////////////////////////////////////////
void CMboxTreeCtrl::ImapHandleDblclkFolder(HTREEITEM h_selitem, UINT nFlags, CPoint point, CMboxTreeItemData* p_itemdata)
{
	QCImapMailboxCommand		*pImapCommand;

	// All should be well but just in case!!
	ASSERT (p_itemdata != NULL);

	pImapCommand = (QCImapMailboxCommand *) p_itemdata->m_pCommand;
	if (!pImapCommand)
	{
		ASSERT (0);
		return;
	}

	// If this is a selectable mailbox, don't expand it. Just open
	// the mailbox. User will have to use the +/- to expand.
	if (pImapCommand->CanContainMessages())
	{
		pImapCommand->Execute( CA_OPEN );
	}
	else
	{
		// Do this for those that can have children but don't have any now.
		if ( ( pImapCommand->CanHaveChildren() ) && ( !ItemHasChildren(h_selitem) ) )
		{
			//
			// User wants to open or close an empty folder.  This
			// case requires special handling since the default behavior
			// of the tree control is to sit there and do nothing.
			//
			int image_idx = -1;
			int unused;
			GetItemImage(h_selitem, image_idx, unused);
			switch (image_idx)
			{
				// Ordinary [read-write] container.
				case QCMailboxImageList::IMAGE_IMAP_FOLDER_OPEN: 
					SetItemImage(h_selitem, QCMailboxImageList::IMAGE_IMAP_FOLDER, QCMailboxImageList::IMAGE_IMAP_FOLDER);
					break;

				case QCMailboxImageList::IMAGE_IMAP_FOLDER: 
					SetItemImage(h_selitem, QCMailboxImageList::IMAGE_IMAP_FOLDER_OPEN, QCMailboxImageList::IMAGE_IMAP_FOLDER_OPEN);
					break;

				// [read-only] container
				case QCMailboxImageList::IMAGE_IMAP_FOLDER_OPEN_READ:
					SetItemImage(h_selitem, QCMailboxImageList::IMAGE_IMAP_FOLDER_READ, QCMailboxImageList::IMAGE_IMAP_FOLDER_READ);
					break;

				case QCMailboxImageList::IMAGE_IMAP_FOLDER_READ:
					SetItemImage(h_selitem, QCMailboxImageList::IMAGE_IMAP_FOLDER_OPEN_READ, QCMailboxImageList::IMAGE_IMAP_FOLDER_OPEN_READ);
					break;

				default:
					// Do default behavior.
					CTreeCtrl::OnLButtonDblClk(nFlags, point);
					break;
			}
		}
		else
		{
			// Do default behavior:
			// Otherwise, toggle expand/contract
			CTreeCtrl::OnLButtonDblClk(nFlags, point);
		}
	}
}



////////////////////////////////////////////////////////////////////////
// ImapHandleDblclkFolder [protected]
//
// This is called when a an attempt is made to open an IMAP folder icon. We need to
// display any children BUT we may need to open this as a mailbox as well.
//  
////////////////////////////////////////////////////////////////////////
void CMboxTreeCtrl::ImapHandleDblclkAccount(HTREEITEM h_selitem, UINT nFlags, CPoint point, CMboxTreeItemData* p_itemdata)
{

	if (ItemHasChildren(h_selitem))
	{
		if (nFlags)
		{
			//
			// This is a hack.  If there is a non-zero
			// nFlags, then we know that the MFC
			// framework called us to handle a real
			// double-click message.  In this case, let the
			// tree control do its default double-click
			// behavior of expanding/collapsing parent items.
			//
			QCTreeCtrl::OnLButtonDblClk(nFlags, point);
			return;
		}

		//
		// If we get this far, this is the other half of the
		// hack.  If we get a zero nFlags value, then we know
		// we're faking a double-click via the Enter key or
		// the Open context menu command.  So, fool the
		// control into thinking it gets an expand/collapse
		// command.
		//

		//
		// First, save the item's current expand/collapse state, then
		// change the item's expand/collapse state.
		//
		UINT oldstate = GetItemState(h_selitem, TVIS_EXPANDED);
		SendMessage(TVM_EXPAND, WPARAM(TVE_TOGGLE), LPARAM(h_selitem)); 

		//
		// Okay, the TVM_EXPAND message does not send the corresponding
		// TVN_ITEMEXPANDED notification message like you might
		// expect, so we have to do it ourselves.  Whatta pain.
		//
		NM_TREEVIEW nmtv;
		if (oldstate & TVIS_EXPANDED)
			nmtv.action = TVE_COLLAPSE;
		else
			nmtv.action = TVE_EXPAND;
		nmtv.itemNew.mask = TVIF_HANDLE | TVIF_PARAM;
		nmtv.itemNew.hItem = h_selitem;
		nmtv.itemNew.lParam = GetItemData(h_selitem);
		LRESULT unused;
		OnItemExpanded((NMHDR* ) &nmtv, &unused);
	}
}




// ========= Additional handlers for new menu items, etc. ==============


////////////////////////////////////////////////////////////////////////
// OnCmdModifyImapAccount [protected]
//
// Context menu command handler for the New command.
////////////////////////////////////////////////////////////////////////
void CMboxTreeCtrl::OnCmdModifyImapAccount(void)
{
	// In case we need to modify downloading option from here!!!
}



////////////////////////////////////////////////////////////////////////
// OnCmdImapRefresh [protected]
//
// Context menu command handler for the "Refresh Mailbox List" command.
// Operation: Go fetch the new list into a temporary file then lock and rename it.
//			  If successful, inform mailbox tree that we need to re-read the mailbox list.
// 
////////////////////////////////////////////////////////////////////////
void CMboxTreeCtrl::OnCmdImapRefresh(void)
{
	HTREEITEM h_item = GetSelectedItem();
	BOOL		ViewNeedsRefresh = FALSE;

	if (NULL == h_item)
		return;

	CMboxTreeItemData* p_itemdata = (CMboxTreeItemData *) GetItemData(h_item);
	ASSERT(p_itemdata != NULL);

	// MUST have a command object.
	QCImapMailboxCommand *pImapCommand =  (QCImapMailboxCommand *)p_itemdata->m_pCommand;
	ASSERT (pImapCommand != NULL);

	// Make sure the selected item is one we're interested in.
	// 
	switch (p_itemdata->m_itemType)
	{
		case ITEM_IMAP_MAILBOX:
		case ITEM_IMAP_NAMESPACE:
		case ITEM_IMAP_ACCOUNT:
			// 
			// Assume a fore-ground refresh for now.

			// Note: On input to Execute(), set the value of the BOOL pData points to
			// to TRUE to indicate that we want to recurse through child mailboxes.
			// It will be set by Execute() to indicate if we need to update the mailbox tree control.
			ViewNeedsRefresh = TRUE;
			pImapCommand->Execute (CA_IMAP_REFRESH, &ViewNeedsRefresh);
			if (ViewNeedsRefresh)
			{
				// Delete all child items and force a re-fresh.
				// Note: Recurse.
				g_theMailboxDirector.UpdateImapMailboxLists (pImapCommand, TRUE, TRUE);
			}
			break;
		default:
			break;
	}
}




////////////////////////////////////////////////////////////////////////
// OnCmdImapResync [protected]
//
// Context menu command handler for the "Synchronize" command.
// 
////////////////////////////////////////////////////////////////////////
void CMboxTreeCtrl::OnCmdImapResync(void)
{
	HTREEITEM h_item = GetSelectedItem();
	BOOL ViewNeedsUpdate = FALSE;

	if (NULL == h_item)
		return;

	CMboxTreeItemData* p_itemdata = (CMboxTreeItemData *) GetItemData(h_item);
	ASSERT(p_itemdata != NULL);

	// MUST have a command object.
	QCImapMailboxCommand *pImapCommand =  (QCImapMailboxCommand *)p_itemdata->m_pCommand;
	ASSERT (pImapCommand != NULL);

	// Make sure the selected item is one we're interested in.
	// 
	switch (p_itemdata->m_itemType)
	{
		case ITEM_IMAP_NAMESPACE:
		case ITEM_IMAP_ACCOUNT:
			// Can't handle these yet.
			break;

		case ITEM_IMAP_MAILBOX:
			pImapCommand->Execute (CA_IMAP_RESYNC, &ViewNeedsUpdate);
			break;
	}

	// Update the mailbox tree view if changes made.
	if (ViewNeedsUpdate)
	{
	}
}



////////////////////////////////////////////////////////////////////////
// OnCmdImapDelete [private]
//
// Context menu command handler for the Delete command of an IMAP object.
////////////////////////////////////////////////////////////////////////
void CMboxTreeCtrl::OnCmdImapDelete(void)
{
	QCImapMailboxCommand *pImapCommand;
	HTREEITEM h_item = GetSelectedItem();
	BOOL	bDeleteChildren;

	// Must have a selected item
	if (NULL == h_item)
		return;

	CMboxTreeItemData* p_itemdata = (CMboxTreeItemData *) GetItemData(h_item);
	ASSERT(p_itemdata != NULL);

	// Must have a command object.
	if ( !p_itemdata->m_pCommand )
	{
		ASSERT (0);
		return;
	}

	//
	// Check for deletable type.
	//
	switch (p_itemdata->m_itemType)
	{
		case ITEM_IMAP_ACCOUNT:
			// Don't allow delete of an account from here.
			return;

		case ITEM_IMAP_MAILBOX:
		case ITEM_IMAP_NAMESPACE:
			break;
		default:
			ASSERT(0);
			return;
	}

	// Get the IMAP command object..
	pImapCommand	= (QCImapMailboxCommand *)p_itemdata->m_pCommand;
	bDeleteChildren	= FALSE;

	// Check for attempt to delete 
	// This will call the IMAP command object's IsEmpty() method.
	if ( ! pImapCommand->IsEmpty() )
	{
		short OldAutoOK = GetIniShort(IDS_INI_AUTO_OK);
		SetIniShort(IDS_INI_AUTO_OK, 0);
		
		// BUG: Must put up our own IMAP dialog!!
		int result = AlertDialog( IDD_DELETE_MBOX_FOLDER, pImapCommand->GetName() );
		SetIniShort(IDS_INI_AUTO_OK, OldAutoOK);
		if (IDCANCEL == result)
			return;

		// Ok. User wants to delete child mailboxes.
		bDeleteChildren = TRUE;
	}

	//
	// Some IMAP servers delete the mailbox's parent also. In that case,
	// we may have to remove local storage for it if no longer exists on the server.
	// So get the parent command before we delete.
	//
	// Note: This is not the best place to be doing this but we don't
	// want to go through a check like this at each level if we are recursing. 
	//
	QCImapMailboxCommand *pParentCommand = g_theMailboxDirector.ImapFindParent(pImapCommand);

	// 
	// Now go delete the requested mailboxfrom the server and locally.
	//
	pImapCommand->Execute( CA_DELETE, &bDeleteChildren );	

	//
	// Now see if the parent was deleted.
	//
	if ( pParentCommand && (pParentCommand->GetType () == MBT_IMAP_MAILBOX) )
	{
		if ( ! g_ImapAccountMgr.MailboxExistsOnServer ( pParentCommand->GetAccountID(), pParentCommand->GetImapName() ) )
		{
			//
			// Remove local cache.
			//
			g_theMailboxDirector.RemoveImapMailboxCommand (pParentCommand, FALSE, TRUE);
		}
	}
			
	// grab keyboard focus back	
	SetFocus();	
}





// Handle the "Filters" menu item.
void CMboxTreeCtrl::OnCmdImapFilters(void)
{
	QCImapMailboxCommand *pImapCommand;
	HTREEITEM h_item = GetSelectedItem();

	// Must have a selected item
	if (NULL == h_item)
		return;

	CMboxTreeItemData* p_itemdata = (CMboxTreeItemData *) GetItemData(h_item);
	ASSERT(p_itemdata != NULL);

	// Must have a command object.
	if ( !p_itemdata->m_pCommand )
	{
		ASSERT (0);
		return;
	}

	//
	// Check for proper type.
	//
	switch (p_itemdata->m_itemType)
	{
		case ITEM_IMAP_MAILBOX:
		{
			// Must be INBOX.
			CString Name;
			Name = p_itemdata->m_pCommand->GetName();
			if (Name.CompareNoCase (CRString (IDS_IMAP_FRIENDLY_INBOX_NAME)) == 0)
			{
				break;
			}
		}
		// If we get here, fall through.
		default:
			ASSERT(0);
			return;
	}

	// Get the IMAP command object. Was chacked for validity above.
	pImapCommand	= (QCImapMailboxCommand *)p_itemdata->m_pCommand;
}



///////////////////////////////////////////////////////////////////////////
// ImapNotify
//
// ImapNotify returns TRUE if it handled the request, otherwise FALSE.
// In the latter case, the event should be passed to the mail mboxtree's
// notify procedure.
// ////////////////////////////////////////////////////////////////////////

BOOL CMboxTreeCtrl::ImapNotify( 
QCCommandObject*	pObject,
COMMAND_ACTION_TYPE	theAction,
void*				pData)
{
	QCImapMailboxCommand*	pImapCommand;
	HTREEITEM hItem;

	// Must have a command object.
	if (!pObject)
	{
		ASSERT (0);
		return FALSE;
	}

	if( pObject->IsKindOf( RUNTIME_CLASS( QCImapMailboxCommand ) ) == FALSE )
	{
		return FALSE;
	}

	pImapCommand = ( QCImapMailboxCommand* ) pObject;

	if( theAction == CA_NEW )
	{
		if( pImapCommand->GetType() == MBT_IMAP_ACCOUNT )
		{
			AddImapItem(ITEM_IMAP_ACCOUNT,  
							pImapCommand->GetName(), pImapCommand, FALSE );
		}
		else if( pImapCommand->GetType() == MBT_IMAP_MAILBOX )
		{
			AddImapItem(ITEM_IMAP_MAILBOX,  
							pImapCommand->GetName(), pImapCommand, FALSE );
		}
		else if( pImapCommand->GetType() == MBT_IMAP_NAMESPACE )
		{
			AddImapItem(ITEM_IMAP_NAMESPACE,
							pImapCommand->GetName(), pImapCommand, FALSE );
		}

		// Refresh the parent.
		HTREEITEM hParentItem = ImapGetParentFromPathname( pImapCommand->GetPathname() );
		if (hParentItem)
		{
			QCMailboxTreeCtrl::UpdateImapItemImage (hParentItem, 0);

//FORNOW (JOK)		PostMessage( TVM_EXPAND, WPARAM( TVE_EXPAND ), LPARAM( hParentItem ) ); 
		}

		// Indicate that we handled this.
		return TRUE;
	}


	// For the rest, we must have an hItem.

	hItem = FindPathname( GetRootItem(), pImapCommand->GetPathname() );
		
	if( hItem == NULL )
	{
		// Main handler will take care of this.
		return FALSE;
	}

	// Do rest.

	if( theAction == CA_DELETE ) 
	{
		DeleteItemData( hItem );
		DeleteItem( hItem );	

		// We handled this.
		return TRUE;
	}
	else if( theAction == CA_IMAP_PROPERTIES )
	{
		SetItemText( hItem, ( LPCSTR ) pImapCommand->GetName() );

		// Redisplay the icos as well:
		QCMailboxTreeCtrl::UpdateImapItemImage (hItem, 0);

		// We handled it.				
		return TRUE;
	}
	else if (theAction == CA_IMAP_REFRESH)
	{
		// If the item is an ACCOUNT, expand it.
		if( pImapCommand->GetType() == MBT_IMAP_ACCOUNT )
		{
			PostMessage( TVM_EXPAND, WPARAM( TVE_EXPAND ), LPARAM( hItem ) );
		}
		else
		{
			// Update it's image.
			QCMailboxTreeCtrl::UpdateImapItemImage (hItem, 0);
		}

		// We handled it.
		return TRUE;
	}

	// For all others, let main handler take care of it.
	return FALSE;
}



////////////////////////////////////////////////////////////////////////
// AddImapMenus 
//
// Add the IMAP-specific menu item to the mailbox context menu.
//
////////////////////////////////////////////////////////////////////////
void AddImapMenus (
	HMENU hMenu)
{
	CMenu		menuSrc;
	CMenu		menuDst;
	INT			i;
	INT			iMax;
	UINT		uID;
	CString		szString;
	
	// Sanity.
	if( hMenu == NULL)
	{
		ASSERT( 0 );

		return;
	}

	// Load the IMAP menu items.
	HMENU hImapMenu = QCLoadMenu(IDR_MBOXTREE_IMAPADD_CONTEXT);

	if (hImapMenu)
	{
		// attach to the menu object
		menuSrc.Attach( hImapMenu );

		iMax = menuSrc.GetMenuItemCount();

		// loop through the entries and add them to the given context menu

		for( i = 0; i < iMax ; i++ )
		{
			// get the source menu id & string
			uID = menuSrc.GetMenuItemID( i );
		
			if( uID == 0 )
			{
				// add a separator
				::InsertMenu(hMenu, 0xFFFFFFFF, MF_SEPARATOR | MF_BYPOSITION, 0, NULL );
			}
			else if (uID == -1)
			{
				ASSERT (0);
			}
			else
			{
				// Add the item to the end.
				menuSrc.GetMenuString( i, szString, MF_BYPOSITION );
				::InsertMenu(hMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, uID, szString);
			}
		}

		// Detach the passed-in menu before we exit do tye menu doesn't get destroyed.
		menuSrc.Detach();
	}
}


	
#endif // IMAP4
