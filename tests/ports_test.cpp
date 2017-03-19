/***************************************************************************
  tag: Peter Soetens  Mon Jun 26 13:26:02 CEST 2006  generictask_test.cpp

                        generictask_test_3.cpp -  description
                           -------------------
    begin                : Mon June 26 2006
    copyright            : (C) 2006 Peter Soetens
    email                : peter.soetens@fmtc.be

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "unit.hpp"
#include <InputPort.hpp>
#include <OutputPort.hpp>

#include <TaskContext.hpp>
#include <extras/SlaveActivity.hpp>
#include <extras/SequentialActivity.hpp>
#include <extras/SimulationActivity.hpp>
#include <extras/SimulationThread.hpp>

#include <boost/function_types/function_type.hpp>
#include <OperationCaller.hpp>

#include <rtt-config.h>

#include <memory>

using namespace std;
using namespace RTT;
using namespace RTT::detail;


class EventPortsTC : public TaskContext
{
public:
    bool had_event;
    int  nb_events;
    EventPortsTC(): TaskContext("eptc") { resetStats(); }
    void updateHook()
    {
        nb_events++;
        had_event = true;
    }
    void resetStats() {
        nb_events = 0;
        had_event = false;
    }
};

/**
 * Fixture.
 */
class PortsTestFixture
{
public:
    TaskContext* tc;
    EventPortsTC* tce;
    EventPortsTC* tc2;
    EventPortsTC* tc3;
    ActivityInterface* tsim;
    ActivityInterface* stsim;
    ActivityInterface* slsim;

    PortInterface* signalled_port;
    void new_data_listener(PortInterface* port)
    {
        signalled_port = port;
    }

public:
    PortsTestFixture()
    {
        tc =  new TaskContext( "root", TaskContext::Stopped );
        tce = new EventPortsTC();
        tc2 = new EventPortsTC();
        tc3 = new EventPortsTC();
        tce->setActivity( new SequentialActivity() );
        tc2->setActivity( new SequentialActivity() );
        tc->setActivity( new SimulationActivity(0.001) );
        tc3->setActivity( new SlaveActivity() );
        tsim = tc->getActivity();
        stsim = tc2->getActivity();
        slsim = tc3->getActivity();
        SimulationThread::Instance()->stop();
    }

    ~PortsTestFixture()
    {
    //     if ( tc->getPeer("programs") )
    //         delete tc->getPeer("programs");
        tsim->stop();
        stsim->stop();
        delete tc;
        delete tce;
        delete tc2;
    }
};


// Registers the fixture into the 'registry'
BOOST_FIXTURE_TEST_SUITE(  PortsTestSuite,  PortsTestFixture )

BOOST_AUTO_TEST_CASE( testPortTaskInterface )
{
    InputPort<int> rp1("Port1");
    OutputPort<int> wp1("Port1");
    InputPort<int> rp2("Port2");
    OutputPort<int> wp2("Port2");

    tc->ports()->addPort( wp1 );
    tc->ports()->addPort( rp2 );

    // check adding same port twice.
    tc->ports()->addPort( wp1 );
    {
        // also check adding different port with same name.
        InputPort<double> other_rp("Port1");
        tc->ports()->addPort( other_rp );
        // port will *not* autoremove itself... so do removePort or a crash will follow.
        tc->ports()->removePort( other_rp.getName() );
    }

    // We're adding the above ports to another TC as well.
    // This is not supported behavior, as it will 'overtake' ownership,
     {
#if __cplusplus > 199711L
        unique_ptr<TaskContext> tc1(new TaskContext( "tc", TaskContext::Stopped ));
        unique_ptr<TaskContext> tc2(new TaskContext( "tc2", TaskContext::Stopped ));
#else
        auto_ptr<TaskContext> tc1(new TaskContext( "tc", TaskContext::Stopped ));
        auto_ptr<TaskContext> tc2(new TaskContext( "tc2", TaskContext::Stopped ));
#endif

        tc1->ports()->addPort( rp1 );
        tc1->ports()->addPort( wp2 );
        tc2->ports()->addPort( rp2 );
        tc2->ports()->addPort( wp1 );

        BOOST_CHECK( tc1->connectPorts(tc2.get()) );
        BOOST_CHECK( wp1.connected() );
        BOOST_CHECK( rp1.connected() );
        wp1.write(2);
        int value = 0;
        BOOST_CHECK( rp1.read(value) );
        BOOST_CHECK_EQUAL(2, value);

        BOOST_CHECK( wp2.connected() );
        BOOST_CHECK( rp2.connected() );
        wp2.write(3);
        value = 0;
        BOOST_CHECK( rp2.read(value) );
        BOOST_CHECK_EQUAL(3, value);
    }

    // Tasks have been destroyed, but the ports not. Automatic disconnection
    // is done when port objects are disconnected
    BOOST_CHECK( rp1.connected() );
    BOOST_CHECK( rp2.connected() );
    BOOST_CHECK( wp1.connected() );
    BOOST_CHECK( wp2.connected() );

    // mandatory
    tc->ports()->removePort( wp1.getName() );
    tc->ports()->removePort( rp2.getName() );
}

BOOST_AUTO_TEST_CASE(testPortConnectionInitialization)
{
    OutputPort<int> wp("WriterName", true);
    InputPort<int> rp("ReaderName", ConnPolicy::data(true));

    BOOST_CHECK( wp.createConnection(rp) );
    int value = 0;
    BOOST_CHECK( !rp.read(value) );
    BOOST_CHECK( !wp.getLastWrittenValue(value) );
    wp.write(10);
    BOOST_CHECK( rp.read(value) );
    BOOST_CHECK_EQUAL( 10, value );

    wp.disconnect(&rp);
    BOOST_CHECK( !wp.connected() );
    BOOST_CHECK( !rp.connected() );

    value = 0;
    BOOST_CHECK( wp.getLastWrittenValue(value) );
    BOOST_CHECK_EQUAL( 10, value );
    BOOST_CHECK_EQUAL( 10, wp.getLastWrittenValue() );

    value = 0;
    BOOST_CHECK( wp.createConnection(rp) );
    BOOST_CHECK( rp.read(value) );
    BOOST_CHECK_EQUAL( 10, value );
    //wp.disconnect();
}

BOOST_AUTO_TEST_CASE(testPortSimpleConnections)
{
    OutputPort<int> wp("WriterName");
    InputPort<int> rp("ReaderName", ConnPolicy::data(ConnPolicy::LOCK_FREE, false));

    BOOST_CHECK( !wp.connected() );
    BOOST_CHECK( !rp.connected() );
    {
        int value;
        BOOST_CHECK( !rp.read(value) );
        wp.write(value); // just checking if is works or if it crashes
    }

    BOOST_REQUIRE( wp.createConnection(rp) );
    BOOST_CHECK( wp.connected() );
    BOOST_CHECK( rp.connected() );

    {
        int value = 0;
        BOOST_CHECK( !rp.read(value) );
        wp.write(1);
        BOOST_CHECK( rp.read(value) );
        BOOST_CHECK( 1 == value );
    }

    rp.clear();
    {
        int value = 0;
        BOOST_CHECK( !rp.read(value) );
        wp.write(1);
        BOOST_CHECK( rp.read(value) );
        BOOST_CHECK( 1 == value );
    }

    // Try disconnecting starting at the writer. Disconnecting from the reader
    // will be done later
    wp.disconnect();
    BOOST_CHECK( !wp.connected() );
    BOOST_CHECK( !rp.connected() );
    {
        int value;
        wp.write(value);
        BOOST_CHECK( !rp.read(value) );
    }
    wp.disconnect(); // calling it when not connected should be fine as well

    {
        int value = 0;
        BOOST_CHECK( !rp.read(value) );
        wp.createBufferConnection(rp, 4);
        BOOST_CHECK( !rp.read(value) );
        wp.write(1);
        wp.write(2);
        wp.write(3);
        wp.write(4);
        wp.write(5);
        BOOST_CHECK_EQUAL(0, value);
        BOOST_CHECK( rp.read(value) );
        BOOST_CHECK_EQUAL(1, value);
        BOOST_CHECK( rp.read(value) );
        BOOST_CHECK_EQUAL(2, value);

        rp.clear();
        BOOST_CHECK_EQUAL( rp.read(value), NoData );
        wp.write(10);
        wp.write(20);
        BOOST_CHECK( rp.read(value) );
        BOOST_CHECK_EQUAL(10, value);
        BOOST_CHECK( rp.read(value) );
        BOOST_CHECK_EQUAL(20, value);
        BOOST_CHECK_EQUAL( rp.read(value), OldData );
    }

    // Try disconnecting from the reader this time
    rp.disconnect();
    BOOST_CHECK( !wp.connected() );
    BOOST_CHECK( !rp.connected() );
    {
        int value;
        wp.write(value);
        BOOST_CHECK( !rp.read(value) );
    }
    rp.disconnect(); // calling it when not connected should be fine as well

    // Test automatic disconnection because of port destruction
    {
        InputPort<int> rp("ReaderName", ConnPolicy::data());
        BOOST_CHECK(wp.createConnection(rp));
        BOOST_CHECK( wp.connected() );
        BOOST_CHECK( rp.connected() );
    }
    BOOST_CHECK( !wp.connected() );
}

BOOST_AUTO_TEST_CASE(testPortOneWriterThreeReaders)
{
    OutputPort<int> wp("W");
    InputPort<int> rp1("R1", ConnPolicy::data());
    InputPort<int> rp2("R2", ConnPolicy::buffer(4));
    InputPort<int> rp3("R3", ConnPolicy::data());

    wp.createConnection(rp1);
    BOOST_CHECK( wp.connected() );
    BOOST_CHECK( rp1.connected() );
    BOOST_CHECK( !rp2.connected() );
    BOOST_CHECK( !rp3.connected() );
    wp.createConnection(rp2);
    BOOST_CHECK( wp.connected() );
    BOOST_CHECK( rp1.connected() );
    BOOST_CHECK( rp2.connected() );
    BOOST_CHECK( !rp3.connected() );
    wp.createConnection(rp3);
    BOOST_CHECK( wp.connected() );
    BOOST_CHECK( rp1.connected() );
    BOOST_CHECK( rp2.connected() );
    BOOST_CHECK( rp3.connected() );

    wp.write(10);
    wp.write(15);
    wp.write(20);
    wp.write(25);
    wp.write(30);

    int value = 0;
    BOOST_CHECK( rp1.read(value));
    BOOST_CHECK_EQUAL(30, value);

    BOOST_CHECK( rp2.read(value));
    BOOST_CHECK_EQUAL(10, value);
    BOOST_CHECK( rp2.read(value));
    BOOST_CHECK_EQUAL(15, value);
    BOOST_CHECK( rp2.read(value));
    BOOST_CHECK_EQUAL(20, value);
    BOOST_CHECK( rp2.read(value));
    BOOST_CHECK_EQUAL(25, value);
    BOOST_CHECK_EQUAL( rp2.read(value), OldData);

    BOOST_CHECK( rp3.read(value));
    BOOST_CHECK_EQUAL(30, value);

    // Now removes only the buffer port
    wp.disconnect(&rp2);
    BOOST_CHECK_EQUAL( rp2.read(value), NoData );
    BOOST_CHECK( wp.connected() );
    BOOST_CHECK( rp1.connected() );
    BOOST_CHECK( !rp2.connected() );
    BOOST_CHECK( rp3.connected() );

    wp.write(10);
    BOOST_CHECK( rp1.read(value));
    BOOST_CHECK_EQUAL(10, value);
    BOOST_CHECK( rp3.read(value));
    BOOST_CHECK_EQUAL(10, value);

    // And finally the other ports as well
    wp.disconnect(&rp1);
    wp.disconnect(&rp3);
    BOOST_CHECK( !wp.connected() );
    BOOST_CHECK( !rp1.connected() );
    BOOST_CHECK( !rp2.connected() );
    BOOST_CHECK( !rp3.connected() );
    BOOST_CHECK_EQUAL( rp1.read(value), NoData );
    BOOST_CHECK_EQUAL( rp2.read(value), NoData );
    BOOST_CHECK_EQUAL( rp3.read(value), NoData );
}

BOOST_AUTO_TEST_CASE(testPortThreeWritersOneReader)
{
    OutputPort<int> wp1("W1");
    OutputPort<int> wp2("W2");
    OutputPort<int> wp3("W3");
    InputPort<int> rp("R", ConnPolicy::buffer(4));

    wp1.createConnection(rp);
    BOOST_CHECK( wp1.connected() );
    BOOST_CHECK( rp.connected() );
    BOOST_CHECK( !wp2.connected() );
    BOOST_CHECK( !wp3.connected() );
    wp2.createConnection(rp);
    BOOST_CHECK( rp.connected() );
    BOOST_CHECK( wp1.connected() );
    BOOST_CHECK( wp2.connected() );
    BOOST_CHECK( !wp3.connected() );
    wp3.createConnection(rp);
    BOOST_CHECK( rp.connected() );
    BOOST_CHECK( wp1.connected() );
    BOOST_CHECK( wp2.connected() );
    BOOST_CHECK( wp3.connected() );

    wp1.write(10);
    wp1.write(20);
    wp1.write(30);
    wp1.write(40);
    wp1.write(50);

    wp2.write(12);
    wp2.write(22);
    wp2.write(32);
    wp2.write(42);
    wp2.write(52);

    wp3.write(13);
    wp3.write(23);
    wp3.write(33);
    wp3.write(43);
    wp3.write(53);

    int value = 0;
    BOOST_CHECK( rp.read(value));
    BOOST_CHECK_EQUAL(10, value);
    BOOST_CHECK( rp.read(value));
    BOOST_CHECK_EQUAL(20, value);
    BOOST_CHECK( rp.read(value));
    BOOST_CHECK_EQUAL(30, value);
    BOOST_CHECK( rp.read(value));
    BOOST_CHECK_EQUAL(40, value);

    // Now removes the middle writer
    wp2.disconnect(&rp);
    BOOST_CHECK( rp.connected() );
    BOOST_CHECK( wp1.connected() );
    BOOST_CHECK( !wp2.connected() );
    BOOST_CHECK( wp3.connected() );

    // write one more sample
    wp1.write(60);
    BOOST_CHECK( rp.read(value));
    BOOST_CHECK_EQUAL(60, value);

    // now check if wp3's connection is used:
    BOOST_CHECK( rp.read(value));
    BOOST_CHECK_EQUAL(13, value);
    BOOST_CHECK( rp.read(value));
    BOOST_CHECK_EQUAL(23, value);

    // in the middle adding a sample
    wp1.write(70);

    BOOST_CHECK( rp.read(value));
    BOOST_CHECK_EQUAL(33, value);
    BOOST_CHECK( rp.read(value));
    BOOST_CHECK_EQUAL(43, value);

    // now the in the middle sample shows up
    BOOST_CHECK_EQUAL( rp.read(value), NewData );
    BOOST_CHECK_EQUAL(70, value);
    value = 0;
    BOOST_CHECK_EQUAL( rp.read(value), OldData );
    BOOST_CHECK_EQUAL(70, value);

    // And finally the other ports as well
    rp.disconnect(&wp1);
    rp.disconnect(&wp3);
    BOOST_CHECK( !rp.connected() );
    BOOST_CHECK( !wp1.connected() );
    BOOST_CHECK( !wp2.connected() );
    BOOST_CHECK( !wp3.connected() );
    BOOST_CHECK_EQUAL( rp.read(value), NoData );
}

BOOST_AUTO_TEST_CASE( testPortObjects)
{
    OutputPort<double> wp1("Write");
    InputPort<double>  rp1("Read");

    tc->ports()->addPort( wp1 );
    tc->ports()->addPort( rp1 );

    // Check if ports were added as objects as well
    BOOST_CHECK( tc->provides("Write") != 0 );
    BOOST_CHECK( tc->provides("Read") != 0 );

    // Set initial value
    wp1.write( 1.0 );

    // Connect ports.
    wp1.createConnection( rp1 );

    // Test OperationCallers set/get
    OperationCaller<void(double const&)> mset;
    OperationCaller<FlowStatus(double&)> mget;

    mset = tc->provides("Write")->getOperation("write");
    BOOST_CHECK( mset.ready() );

    mget = tc->provides("Read")->getOperation("read");
    BOOST_CHECK( mget.ready() );

    mset( 3.991 );

    double get_value = 0;
    BOOST_CHECK( mget(get_value) );
    BOOST_CHECK_CLOSE( 3.991, get_value, 0.001 );

    //// Finally, check cleanup. Ports and port objects must be gone:
    tc->ports()->removePort("Read");
    BOOST_CHECK( tc->provides()->hasService("Read") == 0 );
    BOOST_CHECK( tc->ports()->getPort("Read") == 0 );

    tc->ports()->removePort("Write");
    BOOST_CHECK( tc->provides()->hasService("Write") == 0 );
    BOOST_CHECK( tc->ports()->getPort("Write") == 0 );
}

#ifdef ORO_SIGNALLING_PORTS
BOOST_AUTO_TEST_CASE(testPortSignalling)
{
    OutputPort<double> wp1("Write");
    InputPort<double>  rp1("Read");

    Handle hl( rp1.getNewDataOnPortEvent()->setup(
                boost::bind(&PortsTestFixture::new_data_listener, this, _1) ) );
    hl.connect();

    wp1.createConnection(rp1, ConnPolicy::data());
    signalled_port = 0;
    wp1.write(0.1);
    BOOST_CHECK(&rp1 == signalled_port);

    wp1.disconnect();
    wp1.createConnection(rp1, ConnPolicy::buffer(2));
    signalled_port = 0;
    wp1.write(0.1);
    BOOST_CHECK(&rp1 == signalled_port);
    signalled_port = 0;
    wp1.write(0.1);
    BOOST_CHECK(&rp1 == signalled_port);
    signalled_port = 0;
    wp1.write(0.1);
    BOOST_CHECK(0 == signalled_port);
}
#endif

BOOST_AUTO_TEST_CASE(testPortAddRemove)
{
    OutputPort<double>* wp1 = new OutputPort<double>("Write");
    InputPort<double>*  rp1 = new InputPort<double>("Read");
    InputPort<double>*  ep1 = new InputPort<double>("ERead");
    TaskContext tc("tc");
    tc.addPort( *wp1 );
    tc.addEventPort( *rp1, boost::bind(&PortsTestFixture::new_data_listener, this, _1) );
    tc.addEventPort( *ep1 );

    wp1->createConnection(*rp1, ConnPolicy::data());
    wp1->createConnection(*ep1, ConnPolicy::data());

    tc.start();
    wp1->write(0.1);
    tc.stop();

    tc.ports()->removePort("Write");
    tc.ports()->removePort("Read");
    tc.ports()->removePort("ERead");
    BOOST_CHECK(wp1->getInterface() == 0);
    BOOST_CHECK(rp1->getInterface() == 0);
    BOOST_CHECK(ep1->getInterface() == 0);

    wp1->write(0.1);

    delete wp1;
    delete rp1;
    delete ep1;

    tc.start();
    tc.stop();

    wp1 = new OutputPort<double>("Write");
    rp1 = new InputPort<double>("Read");
    ep1 = new InputPort<double>("ERead");

    tc.addPort( *wp1 );
    tc.addEventPort( *rp1, boost::bind(&PortsTestFixture::new_data_listener, this, _1) );
    tc.addEventPort( *ep1 );

    wp1->createConnection(*rp1, ConnPolicy::data());
    wp1->createConnection(*ep1, ConnPolicy::data());

    tc.start();
    wp1->write(0.1);
    tc.stop();

    tc.ports()->removePort("Write");
    tc.ports()->removePort("Read");
    tc.ports()->removePort("ERead");

    wp1->write(0.1);

    delete wp1;
    delete rp1;
    delete ep1;

    tc.start();
    tc.stop();
}

BOOST_AUTO_TEST_CASE(testEventPortSignalling)
{
    OutputPort<double> wp1("Write");
    InputPort<double>  rp1("Read");

    BOOST_ASSERT(tce->configure());
    BOOST_ASSERT(tce->isConfigured());

    BOOST_ASSERT(slsim->isActive());


    tce->start();
    tce->resetStats();

    tce->addEventPort(rp1,boost::bind(&PortsTestFixture::new_data_listener, this, _1) );

    BOOST_CHECK( slsim->execute() );

    wp1.createConnection(rp1, ConnPolicy::data());
    signalled_port = 0;
    wp1.write(0.1);
    BOOST_CHECK(&rp1 == signalled_port);
    BOOST_CHECK(tce->had_event);
    tce->resetStats();

    wp1.disconnect();
    wp1.createConnection(rp1, ConnPolicy::buffer(2));
    // send two items into the buffer
    signalled_port = 0;
    wp1.write(0.1);
    BOOST_CHECK(&rp1 == signalled_port);
    BOOST_CHECK(tce->had_event);
    tce->resetStats();
    signalled_port = 0;
    wp1.write(0.1);
    BOOST_CHECK(&rp1 == signalled_port);
    BOOST_CHECK(tce->had_event);
    tce->resetStats();
    signalled_port = 0;
    // test buffer full:
    wp1.write(0.1);

    BOOST_CHECK( slsim->execute() );
    BOOST_CHECK(0 == signalled_port);
    BOOST_CHECK( !tce->had_event);
    tce->resetStats();

    // mandatory
    tce->ports()->removePort( rp1.getName() );
}


BOOST_AUTO_TEST_CASE(testEventPortSignallingFromSlave)
{
    OutputPort<double> wp1("Write");
    InputPort<double>  rp1("Read");

    tc3->start();
    tc3->resetStats();

    tc3->addEventPort(rp1,boost::bind(&PortsTestFixture::new_data_listener, this, _1) );

    wp1.createConnection(rp1, ConnPolicy::data());
    signalled_port = 0;
    wp1.write(0.1);

    BOOST_CHECK( slsim->execute() );
    BOOST_CHECK(&rp1 == signalled_port);
    BOOST_CHECK(tc3->had_event);
    tc3->resetStats();

    wp1.disconnect();
    wp1.createConnection(rp1, ConnPolicy::buffer(2));
    // send two items into the buffer
    signalled_port = 0;
    wp1.write(0.1);

    BOOST_CHECK( slsim->execute() );
    BOOST_CHECK(&rp1 == signalled_port);
    BOOST_CHECK(tc3->had_event);
    tc3->resetStats();
    signalled_port = 0;
    wp1.write(0.1);

    BOOST_CHECK( slsim->execute() );
    BOOST_CHECK(&rp1 == signalled_port);
    BOOST_CHECK(tc3->had_event);
    tc3->resetStats();
    signalled_port = 0;
    // test buffer full (updateHook called due to execute, but no callback executed):
    wp1.write(0.1);
    BOOST_CHECK( slsim->execute() );
    BOOST_CHECK(0 == signalled_port);
    BOOST_CHECK( tc3->had_event);
    // empty one element and try again:
    double d;
    rp1.read(d);
    wp1.write(0.1);
    BOOST_CHECK( slsim->execute() );
    BOOST_CHECK(&rp1 == signalled_port);
    tc3->resetStats();

    // mandatory
    tc3->ports()->removePort( rp1.getName() );
}

BOOST_AUTO_TEST_CASE(testPlainPortNotSignalling)
{
    OutputPort<double> wp1("Write");
    InputPort<double>  rp1("Read");

    tce->start();
    tce->resetStats();

    tce->addPort(rp1);

    wp1.createConnection(rp1, ConnPolicy::data());
    wp1.write(0.1);
    BOOST_CHECK( !tce->had_event );
    tce->resetStats();

    wp1.disconnect();
    wp1.createConnection(rp1, ConnPolicy::buffer(2));
    wp1.write(0.1);
    BOOST_CHECK( !tce->had_event );

    // mandatory
    tce->ports()->removePort( rp1.getName() );
}

BOOST_AUTO_TEST_CASE(testPortDataSource)
{
    OutputPort<int> wp1("Write");
#if __cplusplus > 199711L
    unique_ptr<InputPortInterface>
#else
    auto_ptr<InputPortInterface>
#endif
            reader(dynamic_cast<InputPortInterface*>(wp1.antiClone()));
    BOOST_CHECK(wp1.connectTo(&*reader, ConnPolicy::buffer(2)));

    DataSource<int>::shared_ptr source = static_cast< DataSource<int>* >(reader->getDataSource());
    BOOST_CHECK(source);

    BOOST_CHECK(!source->evaluate());
    wp1.write(10);
    wp1.write(20);
    // value is still null when not get()/evaluate()
    BOOST_CHECK_EQUAL(0, source->value());

    // read a sample:
    BOOST_CHECK(source->evaluate());
    BOOST_CHECK_EQUAL(10, source->value());
    BOOST_CHECK_EQUAL(10, source->value());

    // get a sample (=evaluate+value):
    BOOST_CHECK_EQUAL(20, source->get());

    // buffer empty, but value remains same as old:
    BOOST_CHECK(!source->evaluate());
    BOOST_CHECK_EQUAL(0, source->get());
    BOOST_CHECK_EQUAL(20, source->value());
}

BOOST_AUTO_TEST_SUITE_END()

