#include <actor/errorReactionFactory.h>
#include <private/errorReaction.h>

const ErrorReaction *ErrorReactionFactory::restartActor() { return RestartActor::create(); }
const ErrorReaction *ErrorReactionFactory::stopActor() { return StopActor::create(); }
const ErrorReaction *ErrorReactionFactory::stopAllActor() { return StopAllActor::create(); }
const ErrorReaction *ErrorReactionFactory::restartAllActor() {return RestartAllActor::create(); }
const ErrorReaction *ErrorReactionFactory::escalateError() {return EscalateError::create(); }
const ErrorReaction *ErrorReactionFactory::doNothing() {return DoNothingError::create(); }
