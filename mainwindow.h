#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QVector>

namespace Ui {
class MainWindow;
}

struct Overlay {
    QString file_path;
    QString name;
    QGraphicsItem* graphics;
};

class Map {
public:
    Map() = default;
    Map(QString path);

    QString path;
    QString name;
    QVector<Overlay> overlays;
};

class MapScene : public QGraphicsScene
{
    Q_OBJECT

public:
    MapScene(QObject * parent = 0);
    void setMap(Map *m);
    Map *getMap() const;
    void highligt(QPoint pos);
    QGraphicsItem *addOverlay(QString path);
    void deleteOverlay(QGraphicsItem *item);

signals:
     void mousepressed(QMouseEvent*);
     void mousemoved(QMouseEvent*);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent * event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
    QGraphicsRectItem *selectedSquare;
    QGraphicsRectItem *highlightedSquare;
    Map *currentMap;
    QGraphicsPixmapItem *map;

    void addSelectors();
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void pressed(QMouseEvent * e);
    void moved(QMouseEvent * e);
    void calibrated(qreal mx, qreal my, qreal cx, qreal cy);
    void map_selected(QAction *a);
    void overlay_selected(QAction *a);
    void recalculate_manual();
    void update_params();
    
private slots:
    void on_actionCalibrate_triggered();
    void toolbutton_pressed(int id);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_tableWidget_itemSelectionChanged();

    void on_pushButton_4_clicked();

    void on_actionAbout_SS13_Telescience_Manager_triggered();

private:
    Ui::MainWindow *ui;
    MapScene *scene;
    Map *currentMap;
    QButtonGroup* bg;
    qreal mx;
    qreal my;
    qreal cx;
    qreal cy;
    bool selected;
    int sx;
    int sy;

    QVector<Map> maps;
};
Q_DECLARE_METATYPE(Overlay*)

#endif // MAINWINDOW_H
