#ifndef NODESTAB_H
#define NODESTAB_H

#include <QAbstractListModel>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

class PropertyModel : public QAbstractListModel {
    Q_OBJECT
    std::vector<QString> properties;
public:
    virtual int rowCount(const QModelIndex &) const override;
    virtual QVariant data(const QModelIndex & index, int role) const override;
    void recreate();
};

class NodesTab : public QWidget
{
    friend class AppearanceWidget;
    Q_OBJECT
    QGridLayout mainLayout;
    QLabel nodeHeaeder{"<strong>Nodes</strong>"};
    QFrame nodeSeparator;
    QCheckBox allNodeIDsCheck{"Show all node IDs"};
    QCheckBox nodeCommentsCheck{"Show node comments"};
    QCheckBox overrideNodeRadiusCheck{"Override node radius"};
    QDoubleSpinBox nodeRadiusSpin;
    QLabel edgeNodeRatioLabel{"Edge : Node radius ratio"};
    QDoubleSpinBox edgeNodeRatioSpin;
    // property visualization
    QLabel propertyHeader{"<strong>Property Highlighting</strong>"};
    QComboBox propertyCombo;
    PropertyModel propertyModel;
    QDoubleSpinBox propertyMinSpin;
    QDoubleSpinBox propertyMaxSpin;
    QLabel propertyColorLabel{"Color LUT"};
    QString lutFilePath;
    QPushButton propertyLUTButton{"default (…)"};
    QCheckBox propertyUseLutCheck;
    QLabel propertyRadiusLabel{"Radius scale"};
    QDoubleSpinBox propertyRadiusScaleSpin;
    QCheckBox propertyUseRadiusScaleCheck;

    void loadNodeLUTButtonClicked(QString path = "");
public:
    explicit NodesTab(QWidget *parent = 0);

signals:

public slots:
};

#endif // NODESTAB_H
