# coding=utf-8
"""Author: Krzysztof Trzepla
Copyright (C) 2015 ACK CYFRONET AGH
This software is released under the MIT license cited in 'LICENSE.txt'

Brings up a set of onepanel nodes. They can create separate clusters.
"""

import copy
import json
import os

from . import common, docker, dns


def panel_hostname(node_name, instance, uid):
    """Formats hostname for a docker hosting onepanel.
    NOTE: Hostnames are also used as docker names!
    """
    return common.format_hostname([node_name, instance], uid)


def panel_erl_node_name(node_name, instance, uid):
    """Formats erlang node name for a vm on onepanel docker.
    """
    hostname = panel_hostname(node_name, instance, uid)
    return common.format_erl_node_name('onepanel', hostname)


def _tweak_config(config, name, onepanel_instance, uid):
    cfg = copy.deepcopy(config)
    cfg['nodes'] = {'node': cfg['nodes'][name]}

    vm_args = cfg['nodes']['node']['vm.args']
    vm_args['name'] = panel_erl_node_name(name, onepanel_instance, uid)

    return cfg


def _node_up(image, bindir, config, dns_servers, app_name, logdir):
    node_name = config['nodes']['node']['vm.args']['name']

    (name, sep, hostname) = node_name.partition('@')

    command = \
        '''set -e
mkdir -p /root/bin/node/log/
echo 'while ((1)); do chown -R {uid}:{gid} /root/bin/node/log; sleep 1; done' > /root/bin/chown_logs.sh
bash /root/bin/chown_logs.sh &
cat <<"EOF" > /tmp/gen_dev_args.json
{gen_dev_args}
EOF
escript bamboos/gen_dev/gen_dev.escript /tmp/gen_dev_args.json
/root/bin/node/bin/{app_name} console'''
    command = command.format(
        uid=os.geteuid(),
        gid=os.getegid(),
        gen_dev_args=json.dumps({'onepanel': config}),
        app_name=app_name)

    volumes = [(bindir, '/root/build', 'ro')]

    if logdir:
        logdir = os.path.join(os.path.abspath(logdir), hostname)
        volumes.extend([(logdir, '/root/bin/node/log', 'rw')])

    container = docker.run(
        image=image,
        name=hostname,
        hostname=hostname,
        detach=True,
        interactive=True,
        tty=True,
        workdir='/root/build',
        volumes=volumes,
        dns_list=dns_servers,
        privileged=True,
        command=command)

    return (
        {
            'docker_ids': [container],
            'onepanel_nodes': [node_name]
        }
    )


def up(image, bindir, dns_server, uid, config_path, logdir=None):
    config = common.parse_json_config_file(config_path)
    input_dir = config['dirs_config']['onepanel']['input_dir']
    dns_servers, output = dns.maybe_start(dns_server, uid)

    for onepanel_instance in config['onepanel_domains']:
        app_name = config['onepanel_domains'][onepanel_instance]['app_name']

        gen_dev_cfg = {
            'config': {
                'input_dir': input_dir,
                'target_dir': '/root/bin'
            },
            'nodes': config['onepanel_domains'][onepanel_instance]['onepanel']
        }

        configs = [_tweak_config(gen_dev_cfg, node, onepanel_instance, uid)
                   for node in gen_dev_cfg['nodes']]

        for cfg in configs:
            node_out = _node_up(image, bindir, cfg, dns_servers, app_name,
                                logdir)
            common.merge(output, node_out)

    return output
