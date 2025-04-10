#ifndef _CONNECTION_HEADER_HPP_
#define _CONNECTION_HEADER_HPP_ 1
#pragma once

#include <boost/signals2.hpp>

class Connection
{
public:
    virtual ~Connection()
    {
        this->newDataArrived.disconnect_all_slots();
    };

    virtual void stop() = 0;
    virtual bool write(const std::string& msg) = 0;
    virtual void startReadingData() = 0;

    boost::signals2::signal<void(std::vector<char>)> newDataArrived;
};

#endif //!_CONNECTION_HEADER_HPP_