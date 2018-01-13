#ifndef ERROR_REACTION_FACTORY_H__
#define ERROR_REACTION_FACTORY_H__


class ErrorReaction;

class ErrorReactionFactory {
	public:
		static const ErrorReaction *restartActor();
		static const ErrorReaction *restartAllActor();
		static const ErrorReaction *stopActor();
		static const ErrorReaction *stopAllActor();
        static const ErrorReaction *escalateError();
		static const ErrorReaction *doNothing();	
};

#endif