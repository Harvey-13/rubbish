#include<bits/stdc++.h>
using namespace std;

template<typename T>
class Mat{
public:
    friend ostream& operator<<(ostream& out, Mat<T> m){
        for(int i{};i<m.size().first;++i) {
            for (int j{}; j < m.size().second; ++j)
                out << m.at(i, j) << ' ';
            out << endl;
        }
        return out;
    }
    Mat<T> operator*(Mat<T>& o){
        Mat<T> ret(this->size().first, o.size().second);
        for(int i{};i<size().first;++i)
            for(int j{};j<o.size().second;++j)
                for(int k{};k<o.size().first;++k)
                    ret.at(i, j) += at(i, k) * o.at(k , j);
        return ret;
    }
    Mat(int n, int m){
        data_.resize(n, vector<T>(m, 0));
    }
    Mat(initializer_list<vector<T>>&& l){
        data_ = l;
    }
    T& at(int x, int y){
        return data_.at(x).at(y);
    }
    pair<int, int> size(){
        return {data_.size(), data_[0].size()};
    }
    ~Mat(){
        data_.clear();
    }
private:
    vector<vector<T>> data_;
};

template<typename T>
class MatMulTool{
    using task = tuple<Mat<T>, Mat<T>, Mat<T>, shared_ptr<promise<Mat<T>>>>;
public:
    MatMulTool(){
        for(int i{};i<thread::hardware_concurrency();++i) {
            workers_.emplace_back(&MatMulTool::work, this, i);
        }
    }

    void stop(){stop_ = true, sem_.release();}
    future<Mat<T>> enqueue(Mat<T>& a, Mat<T>& b){
        lock_guard<mutex> lock(mtx_);
        auto ret = make_shared<promise<Mat<T>>>();
        tasks_.emplace(a, b, Mat<T>(a.size().first, b.size().second), ret);
        sem_.release();
        return ret->get_future();
    }

    ~MatMulTool(){
        stop_ = true;
        for(auto&t:workers_)t.join();
    }
private:
    void work(int id){
        for(;;){
            if(!id)sem_.acquire();
            bg_.arrive_and_wait();
            if(stop_)return;

            auto&[a,b,c,p] = tasks_.front();
            int batch = a.size().first/ kThreadNum;
            int l = id * batch, r = (id + 1) * batch;
            if(id == kThreadNum - 1) r = a.size().first;

            for(int i{l};i<r;++i)
                for(int j{};j<b.size().second;++j)
                    for(int k{};k<b.size().first;++k)
                        c.at(i, j) += a.at(i, k) * b.at(k, j);

            ed_.arrive_and_wait();
            if(!id){
                lock_guard<mutex> lock(mtx_);
                p->set_value(std::move(c));
                tasks_.pop();
            }
        }
    }
    vector<thread> workers_;
    queue<task> tasks_;
    counting_semaphore<INT16_MAX> sem_{0};
    const unsigned kThreadNum{thread::hardware_concurrency()};
    barrier<> bg_{kThreadNum}, ed_{kThreadNum};
    mutex mtx_;
    bool stop_{};
};

int main(){
    ios::sync_with_stdio(0);
    cin.tie(0), cout.tie(0);
    int size = 1<<12;
    Mat<int> a(size, size);
    Mat<int> b(size, size);
    for(int i{};i<(size);++i)
        for(int j{};j<(size);++j)
            a.at(i, j) = b.at(i, j) = 1;

//    cout << a*b << endl;
    MatMulTool<int> tool;
    vector<future<Mat<int>>> res;
    cout << tool.enqueue(a, b).get();
//    for(int i{};i<10;++i)res.emplace_back(tool.enqueue(a, b));
//    for(int i{};i<10;++i)cout << res[i].get() << endl;
    tool.stop();
    return 0;
}
