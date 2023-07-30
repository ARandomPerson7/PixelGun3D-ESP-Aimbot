/* 
    Created by ARandomPerson
    I have no idea what pixel gun version that is for.
    Most of this code is pretty shit but doesn't matter cuz who tf gonna look at it anyways
*/

monoList<void **> *(*PlayerList)(void *networkStartTable);
bool (*isEnemy)(void *instance, void *otherPlayer);
bool (*isDead)(void *instance);
void *(*playerDamageableGetTranform)(void *instance);
bool (*GetIsFiring)(void *instance);

//Some of the things in here are not even used
struct pg_me_t {
    void* instance;
    void* networkStartTable;
    void* playerDamageable;
    void* firstPersonControl;
    void* cameraPivot;
    void* fpsCameraController;
    void* transform;
    void *transform2;
    void* camera;
    void *baseCamera;
    Vector3 location;
    Quaternion rotation;
    bool firing;
    bool aiming;
};

void *otherNetworkTable = nullptr;
pg_me_t* pg_me;

bool get_isMine(void *player) {
    return *(bool *)((uint64_t)player + 0x5B2);
}

bool get_isEnemy(void* instance, void* otherPlayer) {
    return isEnemy(instance, otherPlayer);
}
bool get_isDead(void* instance) {
    return isDead(instance);
}

void* get_PlayerDamageable(void* player) {
    return *(void **)((uint64_t)player + 0x638);
}

void* get_BaseCamera(void* player) {
    void* skinName = *(void **)((uint64_t)player + 0x660);
    if (skinName != nullptr) {
        void *firstPersonControl = *(void **) ((uint64_t) skinName + 0x1E0);
        if (firstPersonControl != nullptr) {
            return *(void **)((uint64_t)firstPersonControl + 0x1A0);
        }
    }
    return nullptr;
}

Quaternion GetRotationFromLocation(Vector3 targetLocation, float y_bias) {
    return Quaternion::LookRotation((targetLocation + Vector3(y_bias, 0, 0)) - pg_me->location, Vector3(0, 1, 0));
}

//Shitty player list implementation
std::vector<void *> get_PlayerList() {
    std::vector<void *> finalList = std::vector<void *>();

    if (otherNetworkTable != nullptr) {
        monoList<void **> *_enemyTeam = PlayerList(otherNetworkTable);

        if (_enemyTeam != nullptr) {
            for (int i = 0; i < _enemyTeam->getSize(); ++i) {
                void *item = _enemyTeam->getItems()[i];
                if (item != nullptr) {
                    finalList.push_back(item);
                }
            }
        }
    }

    if (pg_me->networkStartTable != nullptr) {
        monoList<void **> *_myTeam = PlayerList(pg_me->networkStartTable);

        if (_myTeam != nullptr) {
            for (int i = 0; i < _myTeam->getSize(); ++i) {
                void *item = _myTeam->getItems()[i];
                if (item != nullptr) {
                    finalList.push_back(item);
                }
            }
        }
    }

    return finalList;
}

void rotateCamera(Quaternion rot) {
    if (pg_me->firstPersonControl != nullptr && pg_me->cameraPivot != nullptr) {
        (new Transform((new Component(pg_me->firstPersonControl))->get_transform()))->set_rotation(rot);
        (new Transform((new Component(pg_me->fpsCameraController))->get_transform()))->set_rotation(rot);
        (new Transform(pg_me->cameraPivot))->set_rotation(rot);
    }
}

void* getBestTarget() {
    void* bestPlayer = nullptr;
    float bestDistance = conf.aim.aimDistance;
    float bestFov = conf.aim.aimRadius;

    std::vector<void *> players = get_PlayerList();
    if (!players.empty()) {
        for (int i = 0; i < players.size(); ++i) {
            void *player = players.at(i);
            if (player != nullptr) {
                void *playerDamageable = get_PlayerDamageable(player);
                if (playerDamageable != nullptr) {
                    void *transform = playerDamageableGetTranform(playerDamageable);
                    if (transform != nullptr) {
                        bool _isEnemy = isEnemy(playerDamageable, pg_me->instance);

                        if (_isEnemy) {
                            Vector3 location = (new Transform(transform))->get_position();
                            auto cam = new Camera(pg_me->camera);
                            Vector3 w2sPoint = cam->WorldToScreen(location);
                            Vector2 screenCenter = Vector2(glWidth / 2, glHeight / 2);
                            Vector2 to = Vector2(w2sPoint.X, glHeight - w2sPoint.Y);
                            float distance = Vector3::Distance(location, pg_me->location);
                            float crosshairDistance = Vector2::Distance(screenCenter, to);

                            if (w2sPoint.Z > 0) {
                                if (distance < bestDistance) {
                                    bestDistance = distance;
                                }

                                if (crosshairDistance < bestFov) {
                                    bestFov = crosshairDistance;
                                }

                                if (distance == bestDistance && crosshairDistance == bestFov) {
                                    bestPlayer = player;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return bestPlayer;
}

void* getClosestTarget() {
    void* bestPlayer = nullptr;
    float bestDistance = 1000;

    std::vector<void *> players = get_PlayerList();
    if (!players.empty()) {
        for (int i = 0; i < players.size(); ++i) {
            void *player = players.at(i);
            if (player != 0) {
                void *playerDamageable = get_PlayerDamageable(player);
                if (playerDamageable != 0) {
                    void *transform = playerDamageableGetTranform(playerDamageable);
                    if (transform != 0) {
                        bool _isEnemy = isEnemy(playerDamageable, pg_me->instance);

                        if (_isEnemy) {
                            Vector3 location = (new Transform(transform))->get_position();
                            float distance = Vector3::Distance(location, pg_me->location);

                            if (distance < bestDistance) {
                                bestDistance = distance;
                                bestPlayer = player;
                            }
                        }
                    }
                }
            }
        }
    }
    return bestPlayer;
}

void PGAimbot() {
    void *player = nullptr;
    if (conf.aim.aimbotType == AimbotType::Radius) {
        player = getBestTarget();
    } else if (conf.aim.aimbotType == AimbotType::Distance) {
        player = getClosestTarget();
    }

    if (player != nullptr && pg_me->instance != nullptr && pg_me->camera != nullptr && pg_me->firstPersonControl != nullptr) {
        void *playerDamageable = get_PlayerDamageable(player);
        if (playerDamageable != nullptr) {
            void *transform = playerDamageableGetTranform(playerDamageable);
            if (transform != nullptr) {
                Vector3 position = (new Transform(transform))->get_position();

                Quaternion targetRotation = GetRotationFromLocation(position, 0);
                Quaternion newRot = targetRotation;

                if (conf.aim.aimSmooth != 0) {
                    newRot = Quaternion::Slerp(pg_me->rotation, targetRotation, conf.aim.aimSmooth);
                }

                if (conf.aim.fireCheck) {
                    if (pg_me->firing) {
                        rotateCamera(newRot);
                    }
                } else {
                    rotateCamera(newRot);
                }
            }
        }
    }
}

void (*old_PlayerMoveCUpdate)(void *instance);
void PlayerMoveCUpdate(void *instance) {
    if (instance != nullptr) {
        if (get_isMine(instance)) {
            conf.esp.enableESP = true;
            pg_me->instance = instance;
            pg_me->networkStartTable = *(void **)((uint64_t)instance + 0x650);
            pg_me->playerDamageable = get_PlayerDamageable(instance);
            pg_me->firing = GetIsFiring(instance);

            void* skinName = *(void **)((uint64_t)instance + 0x660);
            if (skinName != nullptr) {
                pg_me->firstPersonControl = *(void **)((uint64_t)skinName + 0x1E0);
                if (pg_me->firstPersonControl != nullptr) {
                    pg_me->cameraPivot = *(void **) ((uint64_t) get_BaseCamera(instance) + 0x48);
                    pg_me->fpsCameraController = *(void **) ((uint64_t) pg_me->firstPersonControl + 0x18);
                    pg_me->transform2 = (new Component(pg_me->firstPersonControl))->get_transform();

                    if (pg_me->cameraPivot != nullptr) {
                        pg_me->rotation = (new Transform(pg_me->cameraPivot))->get_rotation();
                    }
                }
            }

            if (pg_me->playerDamageable != nullptr) {
                pg_me->transform = playerDamageableGetTranform(pg_me->playerDamageable);
                if (pg_me->transform != nullptr) {
                    auto trans = new Transform(pg_me->transform);
                    pg_me->location = trans->get_position_Injected();
                    void *baseCam = get_BaseCamera(instance);
                    if (baseCam != nullptr) {
                        pg_me->baseCamera = baseCam;
                        pg_me->camera = *(void **) ((uint64_t) baseCam + 0x78);
                        if (pg_me->camera != nullptr) {

                            if (conf.aim.aimbot) {
                                PGAimbot();
                            }
                        }
                    }
                }
            }
        } else {
            //This is shitty but works idc
            if (pg_me->instance != nullptr) {
                void *damageable = get_PlayerDamageable(instance);
                if (damageable != nullptr) {
                    if (get_isEnemy(damageable, pg_me->instance) && !get_isDead(damageable)) {
                        otherNetworkTable = *(void **)((uint64_t)instance + 0x650);
                    }
                }
            }
        }
    }
    old_PlayerMoveCUpdate(instance);
}

void(*old_Player_move_c_destroy)(void *instance);
void Player_move_c_destroy(void *instance) {
    if (instance != nullptr) {
        conf.esp.enableESP = false;
    }
    old_Player_move_c_destroy(instance);
}

void draw_menu() {
	//Draw ESP
	if (conf.esp.enableESP) {
	    std::vector<void *> players = get_PlayerList();
	    if (!players.empty()) {
		for (int i = 0; i < players.size(); ++i) {
		    void *player = players.at(i);
		    if (player != nullptr && pg_me->instance != nullptr && pg_me->transform != nullptr && pg_me->camera != nullptr && pg_me->firstPersonControl != nullptr) {
		        void *playerDamageable = get_PlayerDamageable(player);
		        if (playerDamageable != nullptr) {
		            void *transform = playerDamageableGetTranform(playerDamageable);
		            if (transform != nullptr) {
		                Vector3 location = (new Transform(transform))->get_position();
		                int distance = Vector3::Distance(location, pg_me->location);
		                bool _isEnemy = isEnemy(playerDamageable, pg_me->instance);

		                if (distance >= 500) {
		                    continue;
		                }

		                //ESP
		                if (conf.esp.espLines || conf.esp.espBoxes || conf.esp.espDistance) {
		                    auto cam = new Camera(pg_me->camera);
		                    Vector3 w2sPoint = cam->WorldToScreen(location);
		                    if (w2sPoint.Z > 0) {
		                        Vector3 w2s = cam->WorldToScreen(location - Vector3(0, 1.1, 0));
		                        Vector3 head = cam->WorldToScreen(location + Vector3(0, 1, 0));
		                        Vector2 to = Vector2(head.X, glHeight - head.Y);

		                        ImVec4 espColor;
		                        if (_isEnemy) {
		                            espColor = drawingColorEnemy;
		                        } else {
		                            espColor = drawingColorTeam;
		                        }

		                        if (conf.esp.espLines) {
		                            ESP::DrawLine(ImVec2(glWidth / 2, 0), ImVec2(head.X, glHeight - head.Y), espColor);
		                        }
		                        if (conf.esp.espBoxes) {
		                            float boxHeight = abs(head.Y - w2s.Y);
		                            float boxWidth = boxHeight * 0.65f;
		                            Vector2 vBox = {head.X - (boxWidth / 2), head.Y};

		                            ESP::DrawFilledRect(vBox.X, glHeight - vBox.Y, boxWidth, boxHeight, espColor);
		                        }
		                        if (conf.esp.espDistance) {
		                            char dispsize[100];
		                            sprintf(dispsize, "%d m", distance);
		                            ESP::DrawText(ImVec2(to.X, to.Y), espColor, dispsize);
		                        }
		                    }
		                }
		            }
		        }
		    }
		}
	    }
	}
}
