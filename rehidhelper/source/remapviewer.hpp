#include <vector>
#include <3ds.h>

class RemapViewer
{
    public:
        void Initialize();
        void HandOverUI() {};
        void Controls() {};
        void Finalize() {};
    private:
        std::vector<u64> m_titles;
};