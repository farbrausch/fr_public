// This code is in the public domain. See LICENSE for details.

#include "stdafx.h"
#include "undoBuffer.h"
#include "debug.h"
#include <vector>

// ---- frUndoCommand

frUndoCommand::frUndoCommand(sInt type)
	: m_type(type)
{
}

frUndoCommand::~frUndoCommand()
{
}

sBool frUndoCommand::mergeWith(frUndoCommand* b)
{
	return sFALSE;
}

// ---- frUndoBuffer

struct frUndoBuffer::privateData
{
	std::vector<frUndoCommand*>	m_commands;
	sInt												m_position;

	privateData()
		: m_position(0)
	{
	}
};

frUndoBuffer::frUndoBuffer()
{
	m_priv = new privateData;
}

frUndoBuffer::~frUndoBuffer()
{
	purge();

	delete m_priv;
	m_priv = 0;
}

void frUndoBuffer::add(frUndoCommand* command)
{
	purgeRedo();

	// add the command
	frUndoCommand* lastCommand = m_priv->m_commands.size() ? m_priv->m_commands.back() : 0;

	if (lastCommand && command->m_type == lastCommand->m_type && lastCommand->mergeWith(command))
		delete command;
	else
	{
		m_priv->m_commands.push_back(command);
		m_priv->m_position++;
	}
}

void frUndoBuffer::addAndExecute(frUndoCommand* command)
{
	command->execute();
	add(command);
}

void frUndoBuffer::purgeRedo()
{
	// erase the current redo actions
	while (m_priv->m_commands.size() > m_priv->m_position)
	{
		delete m_priv->m_commands.back();
		m_priv->m_commands.pop_back();
	}
}

void frUndoBuffer::purge()
{
	// erase all actions and set position to the beginning
	while (m_priv->m_commands.size())
	{
		delete m_priv->m_commands.back();
		m_priv->m_commands.pop_back();
	}

	m_priv->m_position = 0;
}

void frUndoBuffer::undo()
{
	if (canUndo())
	{
		fr::debugOut("undoing opcode #%d\n", m_priv->m_commands[m_priv->m_position - 1]->m_type);
		m_priv->m_commands[--m_priv->m_position]->unExecute();
	}
}

void frUndoBuffer::redo()
{
	if (canRedo())
	{
		fr::debugOut("redoing opcode #%d\n", m_priv->m_commands[m_priv->m_position]->m_type);
		m_priv->m_commands[m_priv->m_position++]->execute();
	}
}

sBool frUndoBuffer::canUndo() const
{
	return m_priv->m_position > 0;
}

sBool frUndoBuffer::canRedo() const
{
	return m_priv->m_position < m_priv->m_commands.size();
}
