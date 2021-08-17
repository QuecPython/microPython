__all__ = [
    "subscribe",
    "unsubscribe",
    "publish",
]

import _thread

topic_callback_map = dict()


def sub_table(topic=None):
    if topic is not None:
        return topic_callback_map[topic]
    else:
        return topic_callback_map.copy()


def subscribe(topic, cb):
    """
    subscribe topic and cb
    """
    global topic_callback_map
    if topic is not None and topic not in topic_callback_map:
        topic_callback_map[topic] = list()
    topic_callback_map[topic].append(cb)


def publish(topic, msg):
    """
    publish topic and msg
    """
    cb_list = topic_callback_map.get(topic,list())
    for cb in cb_list:
        _thread.start_new_thread(cb, (topic, msg))


def unsubscribe(topic, cb=None):
    """
    cancel subscribe
    """
    if cb is not None:
        topic_callback_map[topic].remove(cb)
    else:
        del topic_callback_map[topic]

