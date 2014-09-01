#include <common/net/connector_list.h>
#include <time.h>
#include <assert.h>

namespace sails {
namespace common {
namespace net {


ConnectorList::ConnectorList()
    :total(0)
    ,free_size(0)
    ,vConn(NULL)
    ,magic_num(0)
{
}



ConnectorList::~ConnectorList() {
    for (int i = 1; i <= total; i++) {
	vConn[i] = NULL;
    }
    delete[] vConn;
}

void ConnectorList::init(uint32_t size)
{
    total = size;

    free_size  = 0;

    //初始化链接链表
    if(vConn) delete[] vConn;

    //分配total+1个空间(多分配一个空间, 第一个空间其实无效)
    vConn = new std::shared_ptr<Connector>[total+1];

    magic_num = time(NULL);
    magic_num = (((uint32_t)magic_num) << 24) & (0xFFFFFFFF << 24 );
    //free从1开始分配, 这个值为uid, 0保留为管道用
    for(uint32_t i = 1; i <= total; i++)
    {
	vConn[i] = NULL;

	free.push_back(i);

	++free_size;
    }
}

uint32_t ConnectorList::getUniqId()
{
    std::unique_lock<std::mutex> locker(list_mutex);

    uint32_t uid = free.front();
    assert(uid > 0 && uid <= total);
    free.pop_front();
    --free_size;

    return magic_num | uid;
}

std::shared_ptr<Connector> ConnectorList::get(uint32_t uid)
{
    uint32_t magi = uid & (0xFFFFFFFF << 24);
    uid           = uid & (0xFFFFFFFF >> 8);

    if (magi != magic_num) return NULL;

    return vConn[uid];
}

void ConnectorList::add(std::shared_ptr<Connector> connector)
{
    std::unique_lock<std::mutex> locker(list_mutex);

    uint32_t muid = connector->getId();
    uint32_t magi = muid & (0xFFFFFFFF << 24);
    uint32_t uid  = muid & (0xFFFFFFFF >> 8);

    assert(magi ==  magic_num && uid > 0 && uid <= total && vConn[uid]==NULL);

    vConn[uid] = connector;
}

void ConnectorList::del(uint32_t uid)
{
    std::unique_lock<std::mutex> locker(list_mutex);

    uint32_t magi = uid & (0xFFFFFFFF << 24);
    uid           = uid & (0xFFFFFFFF >> 8);

    assert(magi == magic_num && uid > 0 && uid <= total && vConn[uid] != NULL);

    _del(uid);
}

void ConnectorList::_del(uint32_t uid)
{
    assert(uid > 0 && uid <= total && vConn[uid] != NULL);

    vConn[uid] = NULL;

    free.push_back(uid);

    ++free_size;
}

size_t ConnectorList::size()
{
    std::unique_lock<std::mutex> locker(list_mutex);

    return total - free_size;
}



} // namespace net
} // namespace common
} // namespace sails
