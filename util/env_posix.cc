
#include "leveldb/env.h"
#include "port/thread_annotations.h"

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
using namespace std;

namespace leveldb
{

    class PosixEnv : public Env
    {
    public:
        PosixEnv();

        ~PosixEnv() override
        {
            static const char msg[] = "PosixEnv singleton destroyed. Unsupported behavior!\n";
            fwrite(msg, 1, sizeof(msg), stderr);
            abort();
        }

        void Schedule(function<void(void *)> background_work_function, void *background_work_arg) override;

    private:
        mutex _background_work_mutex;
        condition_variable _background_work_cv GUARDED_BY(_background_work_mutex);
        bool _started_background_thread GUARDED_BY(_background_work_mutex);

        /*
         * Stores the work item data in a Schedule() call.
         * Instances are constructed on the thread calling Schedule() and used on the
         * background thread.
         * This structure is thread-safe because it is immutable.
         */
        struct BackgroundWorkItem
        {
            explicit BackgroundWorkItem(function<void(void *)> function, void *arg)
                : work_function(function), arg(arg) {}

            function<void(void *)> work_function;
            void *const arg;
        };

        queue<BackgroundWorkItem> _background_work_queue GUARDED_BY(_background_work_mutex);

        void BackgroundThreadMain();

        static void BackgroundThreadEntryPoint(PosixEnv *env)
        {
            env->BackgroundThreadMain();
        }
    };

    void
    PosixEnv::BackgroundThreadMain()
    {
        while (true)
        {
            function<void(void *)> background_work_function;
            void *background_work_arg;

            {
                unique_lock<mutex> lk(_background_work_mutex);

                // Wait until there is work to be done.
                while (_background_work_queue.empty())
                {
                    _background_work_cv.wait(lk);
                }

                assert(!_background_work_queue.empty());
                background_work_function = _background_work_queue.front().work_function;
                background_work_arg = _background_work_queue.front().arg;
                _background_work_queue.pop();
            }
            background_work_function(background_work_arg);
        }
    }
} // namespace leveldb.