// This code is in the public domain. See LICENSE for details.

#ifndef __RG2_UNDOBUFFER_H_
#define __RG2_UNDOBUFFER_H_

#include "types.h"

class frUndoCommand
{
	sInt		m_type;
	friend class frUndoBuffer;

public:
	frUndoCommand(sInt type);
	virtual ~frUndoCommand();

	virtual void	execute() = 0;
	virtual void	unExecute() = 0;
	virtual sBool	mergeWith(frUndoCommand* b);
};

class frUndoBuffer
{
	struct privateData;
	privateData*	m_priv;

public:
	frUndoBuffer();
	~frUndoBuffer();

	// ---- mutators
	void			add(frUndoCommand* command);
	void			addAndExecute(frUndoCommand* command);
	void			purgeRedo();
	void			purge();

	void			undo();
	void			redo();

	// ---- accessors
	sBool			canUndo() const;
	sBool			canRedo() const;
};

#endif
