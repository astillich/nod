#include <QApplication>
#include <QDebug>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QMenu>
#include <QStyleOptionGraphicsItem>
#include <QUuid>
#include <QUndoStack>
#include <QVector2D>

/*
#include <nodes/DataModelRegistry>
#include <nodes/FlowScene>
#include <nodes/FlowView>
*/

#include "nod/abstractnodemodel.h"
#include "nod/defaultnodeitemfactory.h"
#include "nod/defaultnodeitem.h"
#include "nod/defaultconnectionshape.h"
#include "nod/connectionitem.h"
#include "nod/nodemodel.h"
#include "nod/nodeitem.h"
#include "nod/nodeitemfactory.h"
#include "nod/nodeview.h"
#include "nod/serialized.h"

namespace nod {

class DataFlowModel : public AbstractNodeModel
{
    Q_OBJECT
public:

    using AbstractNodeModel::AbstractNodeModel;

    virtual int                 attributeCount(const NodeID &node) const=0;

    virtual QVariant            attributeData(const NodeID &node, const AttributeID &id, DataRole role)=0;

    virtual void                setAttributeData(const NodeID &node, const AttributeID &id, DataRole role, const QVariant &data);

    /** Assigns an attribute to a port.
     *
     * The attribute changes whenever the port changes.
     *
     */
    virtual void                mapAttribute(const AttributeID &attribute, const PortID &port);

    virtual EditorHint          editorHint(const NodeID &node) const;

};

EditorHint DataFlowModel::editorHint(const NodeID &node) const
{
    Q_UNUSED(node);
    return EditorHint::None;
}

void DataFlowModel::setAttributeData(const NodeID &node, const AttributeID &id, DataRole role, const QVariant &data)
{
    Q_UNUSED(node);
    Q_UNUSED(id);
    Q_UNUSED(role);
    Q_UNUSED(data);
}

void DataFlowModel::mapAttribute(const AttributeID &attribute, const PortID &port)
{
    // TODO
}

namespace qgs { // QGraphicsScene front-end

class WidgetItem : public DefaultNodeItem
{
public:

    WidgetItem(NodeScene &scene, const NodeID &node);
    ~WidgetItem();

    QWidget                     *widget() const;

    void                        setWidget(QWidget *widget);

private:

    QGraphicsProxyWidget        *mProxy = nullptr;
};

WidgetItem::WidgetItem(NodeScene &scene, const NodeID &node)
    : DefaultNodeItem(scene, node)
{
    mProxy = new QGraphicsProxyWidget(this);
}

WidgetItem::~WidgetItem()
{
    delete mProxy;
}

QWidget *WidgetItem::widget() const
{
    return mProxy ? mProxy->widget() : nullptr;
}

void WidgetItem::setWidget(QWidget *widget)
{
    widget->setGeometry(contentRect(boundingRect()).toRect());
    mProxy->setWidget(widget);
}

class DataFlowItem : public WidgetItem
{
public:

    using WidgetItem::WidgetItem;

    /** Creates a widget for an attribute.
     *
     * The widget is used to display / edit the attribute. Derived classes can prevent
     * widget creation by returning null for attributes they want to paint (see drawAttribute).
     *
     */
    virtual QWidget             *createWidget(NodeModel &model, const NodeID &node, const AttributeID &attr);

    virtual void                commitData(QWidget *widget, NodeModel &model, const NodeID &node, const AttributeID &attr);

    /** Draws a single attribute.
     *
     * Derived classes can paint attributes instead of displaying a widget by
     * overriding this method. The default implementation does nothing.
     *
     */
    virtual void                drawAttribute(QPainter &painter, const QRect &rect,
                                             NodeModel &model, const NodeID &node, const AttributeID &attr);

    /** Calculates an attribute rectangle.
     *
     * Note that this does not include the label. The default implementation calculates the
     * rectangle from the widget returned by createWidget().
     *
     */
    virtual QRectF              attributeRect(const QRectF &rc, const AttributeID &attr, int index) const;

    /* NodeItem */

    QSizeF                      calculateContentSize() const override;

    /* DefaultItem */

    void                        drawContent(QPainter &painter, const QRectF &rect) override;

private:
};

QWidget *DataFlowItem::createWidget(NodeModel &model, const NodeID &node, const AttributeID &attr)
{
    return nullptr;
}

void DataFlowItem::commitData(QWidget *widget, NodeModel &model, const NodeID &node, const AttributeID &attr)
{

}

void DataFlowItem::drawAttribute(QPainter &painter, const QRect &rect, NodeModel &model, const NodeID &node, const AttributeID &attr)
{

}

QRectF DataFlowItem::attributeRect(const QRectF &rc, const AttributeID &port, int index) const
{
    return QRectF();
}

QSizeF DataFlowItem::calculateContentSize() const
{
    // TODO: calculate from attribute widgets
    return QSizeF(100, 100);
}

void DataFlowItem::drawContent(QPainter &painter, const QRectF &rect)
{
    //painter.fillRect(rect, Qt::white);
}

class DataFlowScene : public NodeScene
{
    Q_OBJECT
public:

    DataFlowScene(NodeItemFactory &factory, QObject *parent=nullptr)
        : NodeScene(factory, parent) {}
};

}

// namespace qgs

} // namespace nodes

using namespace nod;
using namespace qgs;

struct DefaultPort
{
    NodeID          node;
    PortID          id;
    QString         name;
    Direction       direction;
};

#if 0
// TODO: create a ListModel which handles nodes in a sequential order
// it must be templated and provide node and port template arguments
template <typename T, typename P=DefaultPort>
class ListModel : public AbstractNodeModel
{
public:

private:

    QVector<T>          mPorts;
    QVector<T>          mNodes;
};
#endif

template <typename T>
class NodeListMixin
{
public:

    NodeIt              firstNode() const;

    NodeIt              endNode() const;

    void                nextNode(NodeIt &it) const;

private:

    QVector<T>          mNodes;
};

template <typename T>
class PortListMixin
{
public:

    PortIt              firstPort(const NodeID &node) const;

    PortIt              endPort(const NodeID &node) const;

    void                nextPort(PortIt &it) const;

private:

    QVector<T>          mPorts;
};

class ConnectionMixin
{
public:

    NodeID              connectedNode(const NodeID &node, const PortID &port, PortID *other_port=nullptr) const;

    PortID              connectedPort(const NodeID &node, const PortID &port) const;

    Connection          connection(const NodeID &node, const PortID &port) const;

    bool                canConnect(const NodeID &node1, const PortID &port1,
                                           const NodeID &node2, const PortID &port2) const;

    bool                connect(const NodeID &node1, const PortID &port1,
                                        const NodeID &node2, const PortID &port2);

    bool                disconnect(const NodeID &node);

    bool                disconnect(const NodeID &node, const PortID &port);

    bool                isConnected(const NodeID &node) const;

    bool                isConnected(const NodeID &node, const PortID &port) const;

    bool                isConnected(const Connection &connection) const;

private:

    QVector<Connection> mConnections;
};

template <typename N, typename P=PortListMixin<DefaultPort>, typename C=ConnectionMixin>
class CompositeModel : public AbstractNodeModel
{
public:

    /* NodeModel */

    NodeIt              firstNode() const override { return mNodes.firstNode(); };

    NodeIt              endNode() const override { return mNodes.endNode(); }

    void                nextNode(NodeIt &it) const override { mNodes.nextNode(it); }

    PortIt              firstPort(const NodeID &node) const override { return mPorts.firstPort(node); }

    PortIt              endPort(const NodeID &node) const override { return mPorts.endPort(node); }

    void                nextPort(PortIt &it) const override { return mPorts.nextPort(it); }

    NodeID              connectedNode(const NodeID &node, const PortID &port, PortID *other_port=nullptr) const override { return mConnections.connectedNode(node, port, other_port); }

    PortID              connectedPort(const NodeID &node, const PortID &port) const override { return mConnections.connectedPort(node, port); }

    Connection          connection(const NodeID &node, const PortID &port) const override { return mConnections.connection(node, port); }

    bool                canConnect(const NodeID &node1, const PortID &port1,
                                   const NodeID &node2, const PortID &port2) const override { return mConnections.canConnect(node1, port1, node2, port2); }

    bool                connect(const NodeID &node1, const PortID &port1,
                                const NodeID &node2, const PortID &port2) override { return mConnections.connect(node1, port1, node2, port2); }

    bool                disconnect(const NodeID &node) override { return mConnections.disconnect(node); }

    bool                disconnect(const NodeID &node, const PortID &port) override { return mConnections.disconnect(node, port); }

    bool                isConnected(const NodeID &node) const override { return mConnections.isConnected(node); }

    bool                isConnected(const NodeID &node, const PortID &port) const override { return mConnections.isConnected(node, port); }

    bool                isConnected(const Connection &connection) const override { return mConnections.isConnected(connection); }

private:

    N                   mNodes;
    P                   mPorts;
    C                   mConnections;
};

template <typename T, typename P=PortListMixin<DefaultPort>>
using ListModel = CompositeModel<NodeListMixin<T>, P>;

class TestModel : public DataFlowModel
{
    Q_OBJECT
public:

    struct Port
    {
        PortID          id;
        QString         name;
        Direction       direction;
    };

    struct Node
    {
        NodeID          id;
        QString         name;
        QPointF         position;
        QSizeF          size;

        QVector<Port>   ports;
    };

    QVector<Node>       mNodes;

    using DataFlowModel::DataFlowModel;

    NodeID              createNode(const QString &name)
    {
        NodeID id = { QUuid::createUuid(), { 0 } };
        mNodes.append({ id, name, QPointF(0, 0), QSizeF(0, 0), {} });
        return id;
    }

    PortID              createPort(const NodeID &node, const QString &name, Direction direction)
    {
        int idx = index(node);
        if (idx < 0)
            return PortID::invalid();

        PortID id = { QUuid::createUuid(), { 0 } };
        mNodes[idx].ports.append(Port{ id, name, direction });
        return id;
    }

    void                commitNode(const NodeID &node)
    {
        emit nodeCreated(*this, node);
    }

    int                 index(const NodeID &id) const
    {
        for (int i=0; i<mNodes.size(); ++i)
        {
            if (mNodes[i].id.value == id.value)
                return i;
        }
        return -1;
    }

    int                 index(const NodeID &node_id, const PortID &port) const
    {
        int idx = index(node_id);
        if (idx < 0)
            return -1;

        auto &node = mNodes[idx];
        if (node.id.value == node_id.value)
            return index(idx, port);

        return -1;
    }

    int                 index(int node_index, const PortID &port) const
    {
        auto &node = mNodes[node_index];
        for (int i=0; i<node.ports.size(); ++i)
        {
            if (node.ports[i].id.value == port.value)
                return i;
        }
        return -1;
    }

    NodeIt              firstNode() const override
    {
        if (mNodes.isEmpty())
            return endNode();

        auto &node = mNodes[0];
        return { const_cast<TestModel &>(*this), node.id, 0 };
    }

    NodeIt              endNode() const override
    {
        return { const_cast<TestModel &>(*this), NodeID::invalid(), uint64_t(-1) };
    }

    void                nextNode(NodeIt &it) const override
    {
        if (it == endNode())
            return;

        auto index = it.data() + 1;
        if (index < mNodes.size())
        {
            auto node = mNodes[index];
            it.update(node.id, index);
        } else
            it = endNode();
    }

    PortIt              firstPort(const NodeID &node) const override
    {
        if (!node.isValid())
            return endPort(node);

        int idx = index(node);
        if (idx < 0)
            return endPort(node);

        auto &n = mNodes[idx];
        if (n.ports.isEmpty())
            return endPort(node);

        return { const_cast<TestModel &>(*this), node, uint64_t(idx), n.ports[0].id, 0 };
    }

    PortIt              endPort(const NodeID &node) const override
    {
        return { const_cast<TestModel &>(*this), node, uint64_t(-1), PortID::invalid(), uint64_t(-1) };
    }

    void                nextPort(PortIt &it) const override
    {
        auto end = endPort(it.node());
        if (it == end)
            return;

        auto node_index = it.nodeData();
        if (node_index < mNodes.size())
        {
            auto &node = mNodes[node_index];
            if (!node.ports.isEmpty())
            {
                auto port_index = it.portData();
                if (port_index < node.ports.size() - 1)
                {
                    ++port_index;
                    auto port = node.ports[port_index];
                    it.update(port.id, port_index);
                    return;
                }
            }
        }

        it = end;
    }

    QVariant nodeData(const NodeID &node, DataRole role) const override
    {
        int idx =  index(node);
        if (idx >= 0)
        {
            switch (role)
            {
            case DataRole::Display:
            case DataRole::Name:
                return mNodes[idx].name;
            case DataRole::Position:
                return mNodes[idx].position;
            case DataRole::Size:
                return mNodes[idx].size;
            default:
                break;
            }
        }
        return QVariant();
    }

    void setNodeData(const NodeID &node, const QVariant &value, DataRole role) override
    {
        int idx =  index(node);
        if (idx >= 0)
        {
            switch (role)
            {
            case DataRole::Display:
            case DataRole::Name:
                mNodes[idx].name = value.toString();
                break;
            case DataRole::Position:
                mNodes[idx].position = value.toPointF();
                break;
            case DataRole::Size:
                mNodes[idx].size = value.toSizeF();
            default:
                break;
            }
        }
    }

    QVariant portData(const NodeID &node_id, const PortID &port_id, DataRole role) const override
    {
        int node_idx =  index(node_id);
        if (node_idx < 0)
            return QVariant();

        int port_idx = index(node_id, port_id);
        if (port_idx < 0)
            return QVariant();

        switch (role)
        {
        case DataRole::Display:
        case DataRole::Name:
            return mNodes[node_idx].ports[port_idx].name;
        default:
            break;
        }
        return QVariant();
    }

    Direction portDirection(const NodeID &node_id, const PortID &port_id) const override
    {
        int node_idx =  index(node_id);
        if (node_idx < 0)
            return Direction::Input;

        int port_idx = index(node_id, port_id);
        if (port_idx < 0)
            return Direction::Input;

        return mNodes[node_idx].ports[port_idx].direction;
    }

    NodeFlags flags(const NodeID &node) const override
    {
        return NodeFlag::None;
    }

    int attributeCount(const NodeID &node) const override
    {
        return 0;
    }

    QVariant attributeData(const NodeID &node, const AttributeID &id, DataRole role) override
    {
        return QVariant();
    }
};

class TestNodeFactory : public NodeFactory
{
public:

    NodeID createNode(NodeModel &model, const NodeTypeID &type, const QPointF &position, const NodeID &id) override
    {
        auto &tmodel = reinterpret_cast<TestModel&>(model);
        auto node_id = id.isValid() ? id : tmodel.createNode("test");
        tmodel.setNodeData(node_id, position, DataRole::Position);
        tmodel.commitNode(node_id);
        return node_id;
    }
};

class TestItemFactory : public DefaultNodeItemFactory
{
public:

    using DefaultNodeItemFactory::DefaultNodeItemFactory;

    NodeItem *createNodeItem(const NodeID &node) override
    {
        return new DataFlowItem(scene(), node);
    }
};

#if 0
int TestModel::attributeCount(NodeID node) const
{
    return 4;
}

QVariant TestModel::attributeData(NodeID node, int index, NodeRole role)
{
    switch (role)
    {
    case NodeRole::Display:
        switch (index)
        {
        case 0: return "a";
        case 1: return "b";
        case 2: return "c";
        case 3: return "d";
        }
    default:
        break;
    }

    return QVariant();
}
#endif

class UndoCommand
{
public:

    virtual ~UndoCommand()=0;

    virtual QIcon       icon() const=0;

    virtual QString     name() const=0;

    virtual QString     description() const=0;

    virtual void        undo()=0;

    virtual void        redo()=0;
};

class QtUndoCommand : public QUndoCommand
{
public:

    QtUndoCommand(UndoCommand *cmd)
        : mCmd(cmd)
    {
        setText(cmd->name());
    }

    ~QtUndoCommand()
    {
        delete mCmd;
    }

    void                undo() override
    {
        mCmd->undo();
    }

    void                redo() override
    {
        mCmd->redo();
    }

private:

    UndoCommand         *mCmd = nullptr;
};

class UndoStack
{
public:

    virtual ~UndoStack()=0;

    virtual void        push(UndoCommand *cmd)=0;
};

class DefaultUndoStack : public UndoStack
{
public:


};

class QtUndoStack : public UndoStack
{
public:

    QtUndoStack(QUndoStack &stack)
        : mStack(stack)
    {
    }

    void                push(UndoCommand *cmd) override
    {
        mStack.push(new QtUndoCommand(cmd));
    }

private:

    QUndoStack          &mStack;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QMainWindow w;
    w.show();
    w.resize(1280, 960);

    /*
    auto model_reg = std::make_shared<DataModelRegistry>();

    FlowScene scene(model_reg);
    FlowView *view = new FlowView(&scene, &w);
    w.setCentralWidget(view);
*/
    TestModel model;

    auto node1 = model.createNode(QObject::tr("Bottle"));
    auto node1_port1 = model.createPort(node1, QObject::tr("Beer"), Direction::Output);
    auto node1_port2 = model.createPort(node1, QObject::tr("Rum"), Direction::Output);
    auto node1_port3 = model.createPort(node1, QObject::tr("Vodka"), Direction::Output);
    auto node1_port4 = model.createPort(node1, QObject::tr("Wine"), Direction::Output);
    auto node1_port5 = model.createPort(node1, QObject::tr("Cash"), Direction::Input);
    model.commitNode(node1);

    auto node2 = model.createNode(QObject::tr("Glas"));
    auto node2_port1 = model.createPort(node2, QObject::tr("Alcohol"), Direction::Input);
    auto node2_port2 = model.createPort(node2, QObject::tr("Liquid"), Direction::Output);
    model.commitNode(node2);

    /*
    auto node3 = model.createNode(QObject::tr("Person"));
    auto node3_port1 = model.createPort(node3, QObject::tr("Mouth"), Direction::Input);
    model.commitNode(node3);*/

    model.connect(node1, node1_port1, node2, node2_port1);
    //model.connect(node2, node2_port2, node3, node3_port1);

    model.mNodes[0].position = QPointF(0, 0);
    //model.mNodes[0].size = QSizeF(300, 400);

    model.mNodes[1].position = QPointF(24, 200);

    //model.mNodes[2].position = QPointF(200, 100);
    //model.mNodes[1].size = QSizeF(300, 400);

    Serialized s(false);
    model.serialize(s);

    //qDebug() << qPrintable(s.doc().toJson(QJsonDocument::Indented));

    NodeID node;


    TestNodeFactory node_factory;

    NodeType type;
    type.id.value = QUuid("{74bbc2db-f01b-4144-99c1-b49aefff83cb}");
    type.icon = QIcon(":/icons/package");
    type.name = QObject::tr("Package");
    type.description = QObject::tr("This is a package, it contains various things to be transported from one location to another.");
    node_factory.registerType(type);

    type.id.value = QUuid("{4e3a829c-fd5d-42e8-8ed1-c73724ae20c9}");
    type.icon = QIcon(":/icons/note");
    type.name = QObject::tr("Note");
    type.description = QObject::tr("A note is a piece of text describing the diagram, referencing other documents and other things.");
    node_factory.registerType(type);

    type.id.value = QUuid("{45cb43d2-1ee5-4ad7-ad4c-42516efdf1b8}");
    type.icon = QIcon(":/icons/database");
    type.name = QObject::tr("Database");
    type.description = QObject::tr("The database stores data for all applications of the system.");
    node_factory.registerType(type);

    TestItemFactory item_factory(node_factory);

    DataFlowScene scene(item_factory);

    /*
    auto item = new DefaultItem(scene, node);
    item->setVisible(true);
    //auto item = new DataFlowItem(scene, node);
    scene.addItem(item);
    */

    QUndoStack undo;
    NodeView *view = new NodeView(&undo, &scene, &w);
    w.setCentralWidget(view);
    view->show();

    scene.setModel(&model);

    return app.exec();
}

#include "dataflow.moc"

