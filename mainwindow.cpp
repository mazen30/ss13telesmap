#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "calibrationdialog.h"

#include <QSettings>

/*const int mapTileOffsetX = 26;
const int mapTileOffsetY = 31;

const int mapPixelOffsetLeftX = 32 - 1;  // From left edge
const int mapPixelOffsetTopY = 9; // From top edge
const int mapPixelOffsetBottomY = 32 - 17; // From bottom edge*/

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    scene(new MapScene(this)),
    mx(0),
    my(0),
    cx(0),
    cy(0),
    selected(false),
    sx(0),
    sy(0)
{
    ui->setupUi(this);

    QSettings settings("maps.ini", QSettings::IniFormat);

    QActionGroup *action_group = new QActionGroup(this);

    QStringList map_entries = settings.childGroups();
    for(auto &map: map_entries) {
        settings.beginGroup(map);
        Map m;
        m.path = settings.value("path").toString();
        m.name = map;
        m.mapTileOffsetX = settings.value("mapTileOffsetX").toInt();
        m.mapTileOffsetY = settings.value("mapTileOffsetY").toInt();
        m.mapPixelOffsetLeftX = settings.value("mapPixelOffsetLeftX").toInt();
        m.mapPixelOffsetTopY = settings.value("mapPixelOffsetTopY").toInt();
        m.mapPixelOffsetBottomY = settings.value("mapPixelOffsetBottomY").toInt();
        maps.append(m);
        settings.endGroup();

        QAction *action = new QAction(map,ui->menuMaps);
        action->setData(maps.size() - 1); // Index to the current map
        action->setCheckable(true);
        action->setActionGroup(action_group);
        ui->menuMaps->addAction(action);
    }

    if(maps.size() > 0) {
        currentMap = &(maps[0]);
        scene->setMap(&(maps[0]));
        ui->menuMaps->actions()[0]->setChecked(true);
        setWindowTitle(QString("%1 - %2").arg(QCoreApplication::applicationName()).arg(maps[0].name));
    }

    ui->graphicsView->setScene(scene);
    QApplication::setOverrideCursor( Qt::ArrowCursor );
    ui->graphicsView->show();

    ui->stackedWidget->hide();
    ui->tableWidget->setHorizontalHeaderLabels({"Name", "Map", "X", "Y"});
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);
    ui->tableWidget->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft);
    ui->tableWidget->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setResizeMode(2, QHeaderView::Fixed);
    ui->tableWidget->horizontalHeader()->setResizeMode(3, QHeaderView::Fixed);

    ui->tableWidget->horizontalHeader()->resizeSection(2, 50);
    ui->tableWidget->horizontalHeader()->resizeSection(3, 50);

    QSettings bookmark_settings("bookmarks.ini", QSettings::IniFormat);

    int bookmark_size = bookmark_settings.beginReadArray("bookmark");
    for(int i=0; i < bookmark_size; i++) {
        bookmark_settings.setArrayIndex(i);
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(bookmark_settings.value("name").toString()));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(bookmark_settings.value("map").toString()));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(bookmark_settings.value("x").toString()));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(bookmark_settings.value("y").toString()));
    }
    bookmark_settings.endArray();

    bg = new QButtonGroup(this);
    bg->setExclusive(true);
    bg->addButton(ui->button_favourites);
    bg->setId(ui->button_favourites, 0);
    bg->addButton(ui->button_usage);
    bg->setId(ui->button_usage, 1);
    QObject::connect(bg, SIGNAL(buttonClicked(int)), this, SLOT(toolbutton_pressed(int)));

    QObject::connect(ui->menuMaps, SIGNAL(triggered(QAction*)), this, SLOT(map_selected(QAction*)));
    QObject::connect(scene, SIGNAL(mousepressed(QMouseEvent*)), this, SLOT(pressed(QMouseEvent*)));
    QObject::connect(scene, SIGNAL(mousemoved(QMouseEvent*)), this, SLOT(moved(QMouseEvent*)));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pressed(QMouseEvent *e)
{
    if(e->button() != Qt::RightButton) return;

    selected = true;
    sx = e->x()/32 + currentMap->mapTileOffsetX;
    sy = e->y()/32 + currentMap->mapTileOffsetY;

    ui->label_ox->setText(QString("%1").arg(sx));
    ui->label_oy->setText(QString("%1").arg(sy));

    if(ui->edit_mx->value() != 0 &&
       ui->edit_my->value() != 0) {
        float tx = (static_cast<float>(sx)-cx) / mx;
        float ty = (static_cast<float>(sy)-cy) / my;
        ui->label_tx->setText(QString("%1").arg(tx));
        ui->label_ty->setText(QString("%1").arg(ty));
    }
    e->accept();
}

void MainWindow::moved(QMouseEvent *e)
{
    int x = e->x()/32 + currentMap->mapTileOffsetX;
    int y = e->y()/32 + currentMap->mapTileOffsetY;

    ui->statusBar->showMessage(QString("X: %1 / Y: %2").arg(x).arg(y));

}

MapScene::MapScene(QObject *parent)
    : QGraphicsScene(parent),
      currentMap(nullptr),
      map(nullptr)
{
    QBrush b(QColor(255,0,0,64));
    QPen p;
    p.setColor(QColor(255,0,0,255));
    p.setWidth(2);
    p.setJoinStyle(Qt::RoundJoin);

    selectedSquare = this->addRect(0,0,32,32,p,b);
    selectedSquare->setZValue(101);

    b.setColor(QColor(0,255,0,64));
    p.setColor(QColor(0,64,0,255));

    highlightedSquare = this->addRect(0,0,32,32,p,b);
    highlightedSquare->setZValue(100);
}

void MapScene::setMap(Map *m)
{
    currentMap = m;
    if(map) {
        this->removeItem(map);
        delete map;
    }

    QPixmap map_pix(currentMap->path);
    map = this->addPixmap(map_pix);
    map->setZValue(0);
    setSceneRect(0,0,map_pix.width(), map_pix.height());
    selectedSquare->hide();
    highlightedSquare->hide();
}

Map *MapScene::getMap() const
{
    return currentMap;
}

void MapScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pf(event->scenePos().x() - currentMap->mapPixelOffsetLeftX, height() - event->scenePos().y() - currentMap->mapPixelOffsetBottomY);
    QMouseEvent ne(event->type(), pf.toPoint(), event->button(), event->buttons(), event->modifiers());
    emit mousepressed(&ne);

    if(event->button() == Qt::RightButton) {
//        int x = (event->scenePos().x())/32;
//        int y = (event->scenePos().y() + 16 + currentMap->mapPixelOffsetTopY)/32;
//        int y = (event->scenePos().y() + 32 + currentMap->mapPixelOffsetTopY)/32;

        int x = (event->scenePos().x() - currentMap->mapPixelOffsetLeftX)/32;
        int y = (event->scenePos().y() + 32 - currentMap->mapPixelOffsetTopY)/32;

        selectedSquare->setX(x*32);
        selectedSquare->setY(y*32 - (32 - currentMap->mapPixelOffsetTopY) - 1);
        selectedSquare->show();
    }

    QGraphicsScene::mousePressEvent(event);
}

void MapScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pf(event->scenePos().x() - currentMap->mapPixelOffsetLeftX, height() - event->scenePos().y() - currentMap->mapPixelOffsetBottomY);
    QMouseEvent ne(event->type(), pf.toPoint(), event->button(), event->buttons(), event->modifiers());
    emit mousemoved(&ne);

    int x = (event->scenePos().x() - currentMap->mapPixelOffsetLeftX)/32;
    int y = (event->scenePos().y() + 32 - currentMap->mapPixelOffsetTopY)/32;

    highlightedSquare->setX(x*32 );
    highlightedSquare->setY(y*32 - (32 - currentMap->mapPixelOffsetTopY) - 1);
    highlightedSquare->show();

    QGraphicsScene::mouseMoveEvent(event);
}

void MainWindow::on_actionCalibrate_triggered()
{
    CalibrationDialog d(this);
    QObject::connect(&d, SIGNAL(calibrated(qreal,qreal,qreal,qreal)), this, SLOT(calibrated(qreal,qreal,qreal,qreal)));
    d.exec();
}

void MainWindow::toolbutton_pressed(int id)
{

    if(ui->stackedWidget->isVisible() && ui->stackedWidget->currentIndex() == id) {
        ui->stackedWidget->hide();
        bg->setExclusive(false);
        bg->checkedButton()->setChecked(false);
        bg->setExclusive(true);
    } else {
        ui->stackedWidget->setCurrentIndex(id);
        ui->stackedWidget->show();
    }
}


void MainWindow::calibrated(qreal mx, qreal my, qreal cx, qreal cy)
{
    ui->edit_mx->setValue(mx);
    ui->edit_my->setValue(my);
    ui->edit_cx->setValue(cx);
    ui->edit_cy->setValue(cy);
    update_params();
}

void MainWindow::map_selected(QAction *a)
{
    for(QAction *i: ui->menuMaps->actions())
    {
        i->setChecked(false);
    }

    currentMap = &maps[a->data().toInt()];
    scene->setMap(&maps[a->data().toInt()]);

    a->setChecked(true);
    setWindowTitle(QString("%1 - %2").arg(QCoreApplication::applicationName()).arg(maps[a->data().toInt()].name));
}

void MainWindow::recalculate_manual()
{
    if(ui->edit_mx->value() != 0 &&
       ui->edit_my->value() != 0) {
        qreal mox = ui->edit_mox->value();
        qreal moy = ui->edit_moy->value();
        float tx = (mox-cx) / mx;
        float ty = (moy-cy) / my;
        ui->label_mtx->setText(QString("%1").arg(tx));
        ui->label_mty->setText(QString("%1").arg(ty));
    }
}

void MainWindow::update_params()
{
    mx = ui->edit_mx->value();
    my = ui->edit_my->value();
    cx = ui->edit_cx->value();
    cy = ui->edit_cy->value();
}


Map::Map(QString path, QPoint offsetTopLeft, QPoint offsetBottomRight, QPoint tileOffset)
    : path(path),
      mapTileOffsetX(tileOffset.x()),
      mapTileOffsetY(tileOffset.y()),
      mapPixelOffsetLeftX(offsetTopLeft.x()),
      mapPixelOffsetTopY(offsetTopLeft.y()),
      mapPixelOffsetBottomY(offsetBottomRight.y())
{
}

void MainWindow::on_pushButton_clicked()
{
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
}

void MainWindow::on_pushButton_2_clicked()
{
    ui->tableWidget->removeRow(ui->tableWidget->currentRow());
}

void MainWindow::on_pushButton_3_clicked()
{
    QSettings bookmark_settings("bookmarks.ini", QSettings::IniFormat);
    bookmark_settings.clear();

    bookmark_settings.beginWriteArray("bookmark");
    for(int i=0; i < ui->tableWidget->rowCount(); i++) {
        bookmark_settings.setArrayIndex(i);
        bookmark_settings.setValue("name", ui->tableWidget->item(i, 0)->text());
        bookmark_settings.setValue("map", ui->tableWidget->item(i, 1)->text());
        bookmark_settings.setValue("x", ui->tableWidget->item(i, 2)->text());
        bookmark_settings.setValue("y", ui->tableWidget->item(i, 3)->text());
    }
    bookmark_settings.endArray();

}

void MainWindow::on_tableWidget_itemSelectionChanged()
{
    int row = ui->tableWidget->currentRow();
    if(row == -1) return;

    int bx = ui->tableWidget->item(row, 2)->text().toInt();
    int by = ui->tableWidget->item(row, 3)->text().toInt();

    //ui->edit_mox->setValue(bx);
    //ui->edit_moy->setValue(by);

    if(mx != 0 &&
       my != 0) {
        float tx = (bx-cx) / mx;
        float ty = (by-cy) / my;

        ui->label_bname->setText(ui->tableWidget->item(row, 0)->text());
        ui->label_bmap->setText(ui->tableWidget->item(row, 1)->text());
        ui->label_box->setText(QString("%1").arg(bx));
        ui->label_boy->setText(QString("%1").arg(by));
        ui->label_btx->setText(QString("%1").arg(tx));
        ui->label_bty->setText(QString("%1").arg(ty));
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    if(selected) {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);

        ui->tableWidget->setItem(row, 0, new QTableWidgetItem("Bookmark"));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(currentMap->name));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString("%1").arg(sx)));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString("%1").arg(sy)));
    }
}
