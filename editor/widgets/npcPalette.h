#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QSpinBox>
#include <QLabel>
#include "common/npcType.h"

class NpcPalette : public QWidget
{
    Q_OBJECT

public:
    explicit NpcPalette(QWidget *parent = nullptr);

    NpcType selectedNpc() const;

signals:
    void selectionChanged();

private:
    QRadioButton *_rbNone;
    QRadioButton *_rbPriest;
    QRadioButton *_rbMerchant;
    QRadioButton *_rbBanker;
    QRadioButton *_rbGoblin;
    QRadioButton *_rbSkeleton;
    QRadioButton *_rbZombie;
    QRadioButton *_rbOrc;
    QButtonGroup *_group;

    QRadioButton *_rbGoblinCave;
    QRadioButton *_rbSkeletonCave;
    QRadioButton *_rbSpiderCave;
    QRadioButton *_rbGolemCave;

    QRadioButton *_rbGoblinDungeon;
    QRadioButton *_rbSkeletonDungeon;
    QRadioButton *_rbSpiderDungeon;
    QRadioButton *_rbGolemDungeon;

    QRadioButton *_rbGoblinDesert;
    QRadioButton *_rbSkeletonDesert;
    QRadioButton *_rbSpiderDesert;
    QRadioButton *_rbGolemDesert;
};
