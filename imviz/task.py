from concurrent.futures import ThreadPoolExecutor


THREAD_POOL = ThreadPoolExecutor(32)
TASK_FUTURES = {}


def start(tid, func, *args, **kwargs):
    """
    This calls the given function in a background thread.
    Multiple calls with the same tid will interrupt already running tasks.
    """

    try:
        task_future = TASK_FUTURES[tid]
        if task_future is not None:
            task_future.cancel()
    except KeyError:
        pass

    TASK_FUTURES[tid] = THREAD_POOL.submit(func, *args, **kwargs)


def update(tid, func, *args, **kwargs):
    """
    This calls the given function in a background thread.
    Multiple calls with the same tid will not queue tasks.

    If the task is currently running, no new task will be started,
    nor will the running task be interrupted.
    """

    try:
        task_future = TASK_FUTURES[tid]
    except KeyError:
        task_future = None

    if task_future is None:
        TASK_FUTURES[tid] = THREAD_POOL.submit(func, *args, **kwargs)


def result(tid):
    """
    This returns the task result and resets the task state to "inactive".
    Returns None if no task result is present.
    """

    try:
        task_future = TASK_FUTURES[tid]
        if task_future is None:
            return None
        if not task_future.done():
            return None

        TASK_FUTURES[tid] = None
        return task_future.result()
    except KeyError:
        return None


def cancel(tid):

    try:
        task_future = TASK_FUTURES[tid]
        task_future.cancel()
        TASK_FUTURES[tid] = None
    except KeyError:
        pass


def active(tid):

    try:
        task_future = TASK_FUTURES[tid]
        if task_future is None:
            return False
        else:
            return not task_future.done()
    except KeyError:
        return False
