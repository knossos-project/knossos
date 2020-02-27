/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

#pragma once

#include <QButtonGroup>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QWidget>

#include <bitset>
#include <cmath>

class MSAASpinBox : public QSpinBox {
public:
    MSAASpinBox() {
        setSuffix(tr(" samples"));
        setSpecialValueText("off");
        setRange(0, 64);
    }
    virtual QValidator::State validate(QString &input, int &pos) const override {
        const auto inRange = QSpinBox::validate(input, pos) == QValidator::Invalid;
        const auto number = static_cast<unsigned long>(valueFromText(input));
        const auto valid = inRange && (std::bitset<sizeof(number)>(number).count() <= 1);
        return inRange ? QValidator::Invalid : valid ? QValidator::Acceptable : QValidator::Intermediate;
    }
    virtual void stepBy(int steps) override {
        // we want 0 as value for 2^0 but have to calculate with 1
        const auto oldValue = value() == 0 ? 1 : value();
        const int newValue = std::pow(2, std::floor(std::log2(oldValue)) + steps);
        setValue(newValue == 1 ? 0: newValue);
    }
};

class TreesTab : public QWidget
{
    friend class MainWindow;
    friend class PreferencesWidget;
    Q_OBJECT
    QHBoxLayout mainLayout;
    // tree render options
    QGroupBox renderingGroup{tr("Rendering")};
    QFormLayout renderingLayout;
    QCheckBox highlightActiveTreeCheck{tr("Highlight active tree")};
    QCheckBox highlightIntersectionsCheck{tr("Highlight intersections")};
    QCheckBox lightEffectsCheck{tr("Enable light effects")};
    MSAASpinBox msaaSpin;
    QCheckBox ownTreeColorsCheck{tr("Use custom tree colors")};
    QString lutFilePath;
    QPushButton loadTreeLUTButton{tr("Load …")};
    QDoubleSpinBox depthCutoffSpin;
    QComboBox renderQualityCombo;
    // tree visibility
    QGroupBox visibilityGroup{tr("Tree Visibility")};
    QVBoxLayout visibilityLayout;
    QGroupBox vp3dGroup{tr("In 3D viewport")};
    QVBoxLayout vp3dLayout;
    QButtonGroup vp3dButtonGroup;
    QRadioButton vp3dAllTreesRadio{tr("Show all trees")};
    QRadioButton vp3dSelectedTreesRadio{tr("Show only selected trees")};
    QGroupBox vpOrthoGroup{tr("In othogonal viewports")};
    QVBoxLayout vpOrthoLayout;
    QButtonGroup vpOrthoButtonGroup;
    QRadioButton vpOrthoAllTreesRadio{tr("Show all trees")};
    QRadioButton vpOrthoSelectedTreesRadio{tr("Show only selected trees")};

    void loadTreeLUTButtonClicked(QString path = "");
    void saveSettings(QSettings & settings) const;
    void loadSettings(const QSettings & settings);
public:
    explicit TreesTab(QWidget *parent = nullptr);

signals:

public slots:
    void setTreeVisibility(const bool showIn3d, const bool onlySelectedIn3d, const bool showInOrtho, const bool onlySelectedInOrtho);
};
