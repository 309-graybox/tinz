#include "CharacterStateCrawl.h"

void CharacterStateCrawl::enter(CharacterState *previousState) {}

void CharacterStateCrawl::update(Character *character) {}

void CharacterStateCrawl::exit() {}

CharacterStateType CharacterStateCrawl::type() const
{
    return CharacterStateType::Crawl;
}
