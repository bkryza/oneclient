"""This module tests events emission and subscription using event manager."""

__author__ = "Krzysztof Trzepla"
__copyright__ = """(C) 2015 ACK CYFRONET AGH,
This software is released under the MIT license cited in 'LICENSE.txt'."""

import os
import sys
import time

import pytest

script_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, os.path.dirname(script_dir))
from test_common import *
from performance import *

# noinspection PyUnresolvedReferences
from environment import appmock, common, docker
# noinspection PyUnresolvedReferences
import events
import appmock_client


# noinspection PyClassHasNoInit
class TestCommunicator:
    @classmethod
    def setup_class(cls):
        cls.result = appmock.up(image='onedata/builder', bindir=appmock_dir,
                                dns='none', uid=common.generate_uid(),
                                config_path=os.path.join(script_dir,
                                                         'env.json'))

        [container] = cls.result['docker_ids']
        cls.ip = docker.inspect(container)['NetworkSettings']['IPAddress']. \
            encode('ascii')

    @classmethod
    def teardown_class(cls):
        docker.remove(cls.result['docker_ids'], force=True, volumes=True)

    @performance({
        'repeats': 10,
        'configs': {
            'multiple_events': {
                'description': 'Emits multiple events using event manager.',
            }
        }
    })
    def test_subscription_counter_threshold(self, parameters):
        appmock_client.reset_tcp_server_history(self.ip)
        manager = events.EventManager(1, self.ip, 5555)

        subscription = events.prepareSerializedReadEventSubscription(1, 1, 0,
                                                                     0)
        appmock_client.tcp_server_send(self.ip, 5555, subscription)
        event = manager.emitReadEvent('fileId', 0, 10)
        appmock_client.tcp_server_wait_for_messages(self.ip, 5555, event, 1, 5)

        cancellation = events.prepareSerializedEventSubscriptionCancellation(1)
        appmock_client.tcp_server_send(self.ip, 5555, cancellation)
        event = manager.emitReadEvent('fileId', 0, 10)
        time.sleep(0.5)
        assert 0 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)

    @performance(skip=True)
    def test_subscription_size_threshold(self, parameters):
        manager = events.EventManager(1, self.ip, 5555)

        subscription = events.prepareSerializedReadEventSubscription(1, 0, 0,
                                                                     100)
        appmock_client.tcp_server_send(self.ip, 5555, subscription)
        event = manager.emitReadEvent('fileId', 0, 100)
        time.sleep(0.5)
        assert 1 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)

        cancellation = events.prepareSerializedEventSubscriptionCancellation(1)
        appmock_client.tcp_server_send(self.ip, 5555, cancellation)
        event = manager.emitReadEvent('fileId', 0, 100)
        time.sleep(0.5)
        assert 0 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)

    @performance(skip=True)
    def test_subscription_time_threshold(self, parameters):
        manager = events.EventManager(1, self.ip, 5555)

        subscription = events.prepareSerializedReadEventSubscription(1, 0, 100,
                                                                     0)
        appmock_client.tcp_server_send(self.ip, 5555, subscription)
        event = manager.emitReadEvent('fileId', 0, 1)
        time.sleep(0.5)
        assert 1 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)

        cancellation = events.prepareSerializedEventSubscriptionCancellation(1)
        appmock_client.tcp_server_send(self.ip, 5555, cancellation)
        event = manager.emitReadEvent('fileId', 0, 1)
        time.sleep(0.5)
        assert 0 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)

    @performance(skip=True)
    def test_multiple_subscriptions(self, parameters):
        manager = events.EventManager(1, self.ip, 5555)

        subscription = events.prepareSerializedReadEventSubscription(1, 0, 0,
                                                                     50)
        appmock_client.tcp_server_send(self.ip, 5555, subscription)
        subscription = events.prepareSerializedReadEventSubscription(2, 0, 0,
                                                                     20)
        appmock_client.tcp_server_send(self.ip, 5555, subscription)
        subscription = events.prepareSerializedReadEventSubscription(3, 0, 0,
                                                                     5)
        appmock_client.tcp_server_send(self.ip, 5555, subscription)

        event = manager.emitReadEvent('fileId', 0, 5)
        time.sleep(0.5)
        assert 1 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)
        cancellation = events.prepareSerializedEventSubscriptionCancellation(3)
        appmock_client.tcp_server_send(self.ip, 5555, cancellation)
        event = manager.emitReadEvent('fileId', 0, 20)
        time.sleep(0.5)
        assert 1 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)
        cancellation = events.prepareSerializedEventSubscriptionCancellation(2)
        appmock_client.tcp_server_send(self.ip, 5555, cancellation)
        event = manager.emitReadEvent('fileId', 0, 50)
        time.sleep(0.5)
        assert 1 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)
        cancellation = events.prepareSerializedEventSubscriptionCancellation(1)
        appmock_client.tcp_server_send(self.ip, 5555, cancellation)
        event = manager.emitReadEvent('fileId', 0, 100)
        time.sleep(0.5)
        assert 0 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)

    @performance(skip=True)
    def test_different_subscriptions(self, parameters):
        manager = events.EventManager(1, self.ip, 5555)

        subscription = events.prepareSerializedReadEventSubscription(1, 1, 0,
                                                                     0)
        appmock_client.tcp_server_send(self.ip, 5555, subscription)
        subscription = events.prepareSerializedWriteEventSubscription(2, 1, 0,
                                                                      0)
        appmock_client.tcp_server_send(self.ip, 5555, subscription)

        event = manager.emitReadEvent('fileId', 1, 1)
        time.sleep(0.5)
        assert 1 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)
        cancellation = events.prepareSerializedEventSubscriptionCancellation(1)
        appmock_client.tcp_server_send(self.ip, 5555, cancellation)

        event = manager.emitWriteEvent('fileId', 0, 1, 1)
        time.sleep(0.5)
        assert 1 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)
        event = manager.emitTruncateEvent('fileId', 0)
        time.sleep(0.5)
        assert 1 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)
        cancellation = events.prepareSerializedEventSubscriptionCancellation(2)
        appmock_client.tcp_server_send(self.ip, 5555, cancellation)
        event = manager.emitWriteEvent('fileId', 0, 1, 1)
        time.sleep(0.5)
        assert 0 == appmock_client.tcp_server_message_count(self.ip, 5555,
                                                            event)
