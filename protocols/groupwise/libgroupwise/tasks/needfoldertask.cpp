//
// C++ Implementation: %{MODULE}
//
// Description:
//
//
// Author: %{AUTHOR} <%{EMAIL}>, (C) %{YEAR}
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "client.h"
#include "tasks/createcontactinstancetask.h"
#include "tasks/createfoldertask.h"

#include "needfoldertask.h"

NeedFolderTask::NeedFolderTask(Task* parent): ModifyContactListTask(parent)
{
}

NeedFolderTask::~NeedFolderTask()
{
}

void NeedFolderTask::createFolder()
{
	CreateFolderTask * cct = new CreateFolderTask( client()->rootTask() );
	cct->folder( 0, m_folderSequence, m_folderDisplayName );
	connect( cct, SIGNAL( gotFolderAdded( const FolderItem & ) ), client(), SIGNAL( folderReceived( const FolderItem & ) ) );
	connect( cct, SIGNAL( gotFolderAdded( const FolderItem & ) ), SLOT( slotFolderAdded( const FolderItem & ) ) );
	connect( cct, SIGNAL( finished() ), SLOT( slotFolderTaskFinished() ) );
	cct->go( true );
}

void NeedFolderTask::slotFolderAdded( const FolderItem & addedFolder )
{
	// if this is the folder we were trying to create
	if ( m_folderDisplayName == addedFolder.name )
	{
		qDebug( "NeedFolderTask::slotFolderAdded() - Folder %s was created on the server, now has objectId %i", addedFolder.name.ascii(), addedFolder.id );
		m_folderId = addedFolder.id;
	}
}

void NeedFolderTask::slotFolderTaskFinished()
{
	CreateFolderTask *cct = ( CreateFolderTask* )sender();
	if ( cct->success() )
	{
		// call our child class's action to be performed
		onFolderCreated();
	}
	else
		setError( 1, "Folder creation failed" );
}

#include "needfoldertask.moc"
