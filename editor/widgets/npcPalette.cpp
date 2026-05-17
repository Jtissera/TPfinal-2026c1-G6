#include "npcPalette.h"

NpcPalette::NpcPalette(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    setFixedWidth(160);

    _group = new QButtonGroup(this);

    // Sin NPC
    auto* noneGroup = new QGroupBox("Sin NPC", this);
    auto* noneLayout = new QVBoxLayout(noneGroup);
    _rbNone = new QRadioButton("Ninguno", noneGroup);
    _rbNone->setChecked(true);
    _group->addButton(_rbNone);
    noneLayout->addWidget(_rbNone);
    mainLayout->addWidget(noneGroup);

    // NPCs de ciudad
    auto* cityGroup = new QGroupBox("Ciudad (zona segura)", this);
    auto* cityLayout = new QVBoxLayout(cityGroup);
    _rbPriest   = new QRadioButton("Sacerdote",    cityGroup);
    _rbMerchant = new QRadioButton("Comerciante",  cityGroup);
    _rbBanker   = new QRadioButton("Banquero",     cityGroup);
    _group->addButton(_rbPriest);
    _group->addButton(_rbMerchant);
    _group->addButton(_rbBanker);
    cityLayout->addWidget(_rbPriest);
    cityLayout->addWidget(_rbMerchant);
    cityLayout->addWidget(_rbBanker);
    mainLayout->addWidget(cityGroup);

    // Criaturas
    auto* mobGroup = new QGroupBox("Criaturas (combate)", this);
    auto* mobLayout = new QVBoxLayout(mobGroup);
    _rbGoblin   = new QRadioButton("Goblin",    mobGroup);
    _rbSkeleton = new QRadioButton("Esqueleto", mobGroup);
    _rbZombie   = new QRadioButton("Zombie",    mobGroup);
    _rbGuard    = new QRadioButton("Guardia",   mobGroup);
    _group->addButton(_rbGoblin);
    _group->addButton(_rbSkeleton);
    _group->addButton(_rbZombie);
    _group->addButton(_rbGuard);
    mobLayout->addWidget(_rbGoblin);
    mobLayout->addWidget(_rbSkeleton);
    mobLayout->addWidget(_rbZombie);
    mobLayout->addWidget(_rbGuard);
    mainLayout->addWidget(mobGroup);

    connect(_group,
            static_cast<void(QButtonGroup::*)(QAbstractButton*)>(
                &QButtonGroup::buttonClicked),
            this, &NpcPalette::selectionChanged);
}

NpcType NpcPalette::selectedNpc() const {
    if (_rbPriest->isChecked())   return NpcType::PRIEST;
    if (_rbMerchant->isChecked()) return NpcType::MERCHANT;
    if (_rbBanker->isChecked())   return NpcType::BANKER;
    if (_rbGoblin->isChecked())   return NpcType::GOBLIN;
    if (_rbSkeleton->isChecked()) return NpcType::SKELETON;
    if (_rbZombie->isChecked())   return NpcType::ZOMBIE;
    if (_rbGuard->isChecked())    return NpcType::GUARD;
    return NpcType::NONE;
}
