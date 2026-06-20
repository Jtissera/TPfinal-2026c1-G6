#include "npcPalette.h"

NpcPalette::NpcPalette(QWidget *parent) : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    setFixedWidth(160);

    _group = new QButtonGroup(this);

    // Sin NPC
    auto *noneGroup = new QGroupBox("Sin NPC", this);
    auto *noneLayout = new QVBoxLayout(noneGroup);
    _rbNone = new QRadioButton("Ninguno", noneGroup);
    _rbNone->setChecked(true);
    _group->addButton(_rbNone);
    noneLayout->addWidget(_rbNone);
    mainLayout->addWidget(noneGroup);

    // NPCs de ciudad
    auto *cityGroup = new QGroupBox("Ciudad (zona segura)", this);
    auto *cityLayout = new QVBoxLayout(cityGroup);
    _rbPriest = new QRadioButton("Sacerdote", cityGroup);
    _rbMerchant = new QRadioButton("Comerciante", cityGroup);
    _rbBanker = new QRadioButton("Banquero", cityGroup);
    _group->addButton(_rbPriest);
    _group->addButton(_rbMerchant);
    _group->addButton(_rbBanker);
    cityLayout->addWidget(_rbPriest);
    cityLayout->addWidget(_rbMerchant);
    cityLayout->addWidget(_rbBanker);
    mainLayout->addWidget(cityGroup);

    // Criaturas
    auto *mobGroup = new QGroupBox("Criaturas (combate)", this);
    auto *mobLayout = new QVBoxLayout(mobGroup);
    _rbGoblin = new QRadioButton("Goblin", mobGroup);
    _rbSkeleton = new QRadioButton("Esqueleto", mobGroup);
    _rbZombie = new QRadioButton("Zombie", mobGroup);
    _rbOrc = new QRadioButton("Orco", mobGroup);
    _group->addButton(_rbGoblin);
    _group->addButton(_rbSkeleton);
    _group->addButton(_rbZombie);
    _group->addButton(_rbOrc);
    mobLayout->addWidget(_rbGoblin);
    mobLayout->addWidget(_rbSkeleton);
    mobLayout->addWidget(_rbZombie);
    mobLayout->addWidget(_rbOrc);
    mainLayout->addWidget(mobGroup);

    auto *cavernGroup = new QGroupBox("Caverna", this);
    auto *cavernLayout = new QVBoxLayout(cavernGroup);
    _rbGoblinCave = new QRadioButton("Goblin Caverna", cavernGroup);
    _rbSkeletonCave = new QRadioButton("Esqueleto Caverna", cavernGroup);
    _rbSpiderCave = new QRadioButton("Araña Caverna", cavernGroup);
    _rbGolemCave = new QRadioButton("Golem Caverna", cavernGroup);
    for (auto *rb : {_rbGoblinCave, _rbSkeletonCave, _rbSpiderCave, _rbGolemCave})
    {
        _group->addButton(rb);
        cavernLayout->addWidget(rb);
    }
    mainLayout->addWidget(cavernGroup);

    auto *dungeonGroup = new QGroupBox("Mazmorra", this);
    auto *dungeonLayout = new QVBoxLayout(dungeonGroup);
    _rbGoblinDungeon = new QRadioButton("Goblin Mazmorra", dungeonGroup);
    _rbSkeletonDungeon = new QRadioButton("Esqueleto Mazmorra", dungeonGroup);
    _rbSpiderDungeon = new QRadioButton("Araña Mazmorra", dungeonGroup);
    _rbGolemDungeon = new QRadioButton("Golem Mazmorra", dungeonGroup);
    for (auto *rb : {_rbGoblinDungeon, _rbSkeletonDungeon,
                     _rbSpiderDungeon, _rbGolemDungeon})
    {
        _group->addButton(rb);
        dungeonLayout->addWidget(rb);
    }
    mainLayout->addWidget(dungeonGroup);

    auto *desertGroup = new QGroupBox("Desierto", this);
    auto *desertLayout = new QVBoxLayout(desertGroup);
    _rbGoblinDesert = new QRadioButton("Goblin Desierto", desertGroup);
    _rbSkeletonDesert = new QRadioButton("Esqueleto Desierto", desertGroup);
    _rbSpiderDesert = new QRadioButton("Araña Desierto", desertGroup);
    _rbGolemDesert = new QRadioButton("Golem Desierto", desertGroup);
    for (auto *rb : {_rbGoblinDesert, _rbSkeletonDesert,
                     _rbSpiderDesert, _rbGolemDesert})
    {
        _group->addButton(rb);
        desertLayout->addWidget(rb);
    }
    mainLayout->addWidget(desertGroup);

    connect(_group,
            static_cast<void (QButtonGroup::*)(QAbstractButton *)>(
                &QButtonGroup::buttonClicked),
            this, &NpcPalette::selectionChanged);
}

NpcType NpcPalette::selectedNpc() const
{
    if (_rbPriest->isChecked())
        return NpcType::PRIEST;
    if (_rbMerchant->isChecked())
        return NpcType::MERCHANT;
    if (_rbBanker->isChecked())
        return NpcType::BANKER;
    if (_rbGoblin->isChecked())
        return NpcType::GOBLIN;
    if (_rbSkeleton->isChecked())
        return NpcType::SKELETON;
    if (_rbZombie->isChecked())
        return NpcType::ZOMBIE;
    if (_rbOrc->isChecked())
        return NpcType::ORC;
    if (_rbGoblinCave->isChecked())
        return NpcType::GOBLIN_CAVE;
    if (_rbSkeletonCave->isChecked())
        return NpcType::SKELETON_CAVE;
    if (_rbSpiderCave->isChecked())
        return NpcType::SPIDER_CAVE;
    if (_rbGolemCave->isChecked())
        return NpcType::GOLEM_CAVE;
    if (_rbGoblinDungeon->isChecked())
        return NpcType::GOBLIN_DUNGEON;
    if (_rbSkeletonDungeon->isChecked())
        return NpcType::SKELETON_DUNGEON;
    if (_rbSpiderDungeon->isChecked())
        return NpcType::SPIDER_DUNGEON;
    if (_rbGolemDungeon->isChecked())
        return NpcType::GOLEM_DUNGEON;
    if (_rbGoblinDesert->isChecked())
        return NpcType::GOBLIN_DESERT;
    if (_rbSkeletonDesert->isChecked())
        return NpcType::SKELETON_DESERT;
    if (_rbSpiderDesert->isChecked())
        return NpcType::SPIDER_DESERT;
    if (_rbGolemDesert->isChecked())
        return NpcType::GOLEM_DESERT;
    return NpcType::NONE;
}