#pragma once

#include "person_controller.hpp"

class PersonAnimator {
	PersonCtl* m_ctl;

public:
	PersonAnimator(PersonCtl* ctl) : m_ctl(ctl) {
		//TODO loading assets in AssetManager

		m_ctl->person()->EvAttack.subscribe(
			[this](Person::Damage strength){
				return animateAttack(strength);
			}
		);
	}

private:
	void
	animateAttack(Person::Damage strength)
	{
		//TODO build animation like it have been done in example in anim-viz and do animation.play()
	}
};
