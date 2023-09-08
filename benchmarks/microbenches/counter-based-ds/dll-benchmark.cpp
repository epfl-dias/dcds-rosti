//
// Created by prathamesh on 28/8/23.
//

#include <benchmark/benchmark.h>

#include <dcds/builder/builder.hpp>
#include <dcds/context/DCDSContext.hpp>

class DCDS_Node {
public:
    DCDS_Node(int64_t payload = 0) {
        dcds::DCDSContext context(false, false);
        dcds::Builder dsBuilder(context, "Node");

        dsBuilder.addAttribute("payload", dcds::INTEGER, payload);
        dsBuilder.addAttribute("next_reference", dcds::RECORD_PTR, nullptr);
        dsBuilder.addAttribute("prev_reference", dcds::RECORD_PTR, nullptr);

        storageObject = dsBuilder.initializeStorage();
        storageObjectPtr = reinterpret_cast<void *>(storageObject.get());
    }

    static void initialize();

    auto read_payload() { return built_read_payload_fn(storageObjectPtr); }
    auto read_next_ref() { return built_read_next_ref_fn(storageObjectPtr); }
    auto read_prev_ref() { return built_read_prev_ref_fn(storageObjectPtr); }

    auto write(int64_t *payload, void *next_reference, void *prev_reference) {
        built_write_fn(storageObjectPtr, payload, next_reference, prev_reference);
    }

private:
    static void (*built_write_fn)(void *, int64_t *, void *, void *);
    static void *(*built_read_prev_ref_fn)(void *);
    static void *(*built_read_next_ref_fn)(void *);
    static int64_t (*built_read_payload_fn)(void *);

public:
    static std::shared_ptr<dcds::Visitor> visitor;
    std::shared_ptr<dcds::StorageLayer> storageObject;
    void *storageObjectPtr;
};

void DCDS_Node::initialize() {
    dcds::DCDSContext context(false, false);
    dcds::Builder dsBuilder(context, "Node");

    dsBuilder.addAttribute("payload", dcds::INTEGER, 0);
    dsBuilder.addAttribute("next_reference", dcds::RECORD_PTR, nullptr);
    dsBuilder.addAttribute("prev_reference", dcds::RECORD_PTR, nullptr);

    auto read_payload_fn = dsBuilder.createFunction("read_payload_function", dcds::valueType::INTEGER);
    auto read_next_ref_fn = dsBuilder.createFunction("read_next_ref_function", dcds::valueType::RECORD_PTR);
    auto read_prev_ref_fn = dsBuilder.createFunction("read_prev_ref_function", dcds::valueType::RECORD_PTR);

    auto write_fn = dsBuilder.createFunction("write_function", dcds::valueType::VOID, dcds::valueType::INTEGER,
                                             dcds::valueType::RECORD_PTR, dcds::valueType::RECORD_PTR);

    dsBuilder.addTempVar("node_read_payload_variable", dcds::valueType::INTEGER, 0, read_payload_fn);
    dsBuilder.addTempVar("node_read_next_ref_variable", dcds::valueType::RECORD_PTR, nullptr, read_next_ref_fn);
    dsBuilder.addTempVar("node_read_prev_ref_variable", dcds::valueType::RECORD_PTR, nullptr, read_prev_ref_fn);

    dsBuilder.addArgVar("node_write_payload_variable", dcds::valueType::INTEGER, write_fn);
    dsBuilder.addArgVar("node_write_next_ref_variable", dcds::valueType::RECORD_PTR, write_fn);
    dsBuilder.addArgVar("node_write_prev_ref_variable", dcds::valueType::RECORD_PTR, write_fn);

    auto read_payload_statement =
            dsBuilder.createReadStatement(dsBuilder.getAttribute("payload"), "node_read_payload_variable");
    auto read_next_ref_statement =
            dsBuilder.createReadStatement(dsBuilder.getAttribute("next_reference"), "node_read_next_ref_variable");
    auto read_prev_ref_statement =
            dsBuilder.createReadStatement(dsBuilder.getAttribute("prev_reference"), "node_read_prev_ref_variable");

    auto write_payload_statement =
            dsBuilder.createUpdateStatement(dsBuilder.getAttribute("payload"), "node_write_payload_variable");
    auto write_next_ref_statement =
            dsBuilder.createUpdateStatement(dsBuilder.getAttribute("next_reference"), "node_write_next_ref_variable");
    auto write_prev_ref_statement =
            dsBuilder.createUpdateStatement(dsBuilder.getAttribute("prev_reference"), "node_write_prev_ref_variable");

    auto return_payload_statement = dsBuilder.createReturnStatement("node_read_payload_variable");
    auto return_next_ref_statement = dsBuilder.createReturnStatement("node_read_next_ref_variable");
    auto return_prev_ref_statement = dsBuilder.createReturnStatement("node_read_prev_ref_variable");

    dsBuilder.addStatement(read_payload_statement, read_payload_fn);
    dsBuilder.addStatement(return_payload_statement, read_payload_fn);

    dsBuilder.addStatement(read_next_ref_statement, read_next_ref_fn);
    dsBuilder.addStatement(return_next_ref_statement, read_next_ref_fn);

    dsBuilder.addStatement(read_prev_ref_statement, read_prev_ref_fn);
    dsBuilder.addStatement(return_prev_ref_statement, read_prev_ref_fn);

    dsBuilder.addStatement(write_payload_statement, write_fn);
    dsBuilder.addStatement(write_next_ref_statement, write_fn);
    dsBuilder.addStatement(write_prev_ref_statement, write_fn);

    dsBuilder.addFunction(read_payload_fn);
    dsBuilder.addFunction(read_next_ref_fn);
    dsBuilder.addFunction(read_prev_ref_fn);

    dsBuilder.addFunction(write_fn);

    visitor = dsBuilder.codegen();

    built_read_payload_fn = visitor->getBuiltFunctionInt64ReturnType("read_payload_function");
    built_read_next_ref_fn = visitor->getBuiltFunctionVoidPtrReturnType("read_next_ref_function");
    built_read_prev_ref_fn = visitor->getBuiltFunctionVoidPtrReturnType("read_prev_ref_function");

    built_write_fn = visitor->getBuiltFunctionVoidReturnTypeIntArg1VoidPtr2("write_function");
}

void (*DCDS_Node::built_write_fn)(void *, int64_t *, void *, void *);
void *(*DCDS_Node::built_read_prev_ref_fn)(void *);
void *(*DCDS_Node::built_read_next_ref_fn)(void *);
int64_t (*DCDS_Node::built_read_payload_fn)(void *);
std::shared_ptr<dcds::Visitor> DCDS_Node::visitor;

class Node {
public:
    Node(int64_t payload_ = 0) : payload(payload_) {}

    void write(int64_t *payload_, Node *next_ref_, Node *prev_ref_) {
        m.lock();
        this->payload = *payload_;
        m.unlock();

        m.lock();
        this->next_ref = next_ref_;
        m.unlock();

        m.lock();
        this->prev_ref = prev_ref_;
        m.unlock();
    }

    int64_t read_payload() {
        m.lock();
        auto c = payload;
        m.unlock();
        return c;
    }

    Node *read_prev_ref() {
        m.lock();
        auto c = prev_ref;
        m.unlock();
        return c;
    }

    Node *read_next_ref() {
        m.lock();
        auto c = next_ref;
        m.unlock();
        return c;
    }

private:
    int64_t payload;
    Node *next_ref = nullptr;
    Node *prev_ref = nullptr;
    std::mutex m;
};

class DL_LinkedList {
public:
    void push_front(int64_t payload) {
        auto new_node_ptr = new Node;
        new_node_ptr->write(&payload, nullptr, front);

        if (front != nullptr) {
            auto front_val = front->read_payload();

            front->write(&front_val, new_node_ptr, front->read_prev_ref());
        } else {
            back = new_node_ptr;
        }
        front = new_node_ptr;
        ++size;
    }

    auto pop_back() {
        if (back == nullptr) return static_cast<int64_t>(-1);

        int64_t return_val = back->read_payload();
        if (front != back) {
            back = reinterpret_cast<Node *>(back->read_next_ref());
        } else {
            back = nullptr;
            front = nullptr;
        }

        --size;
        return return_val;
    }

    void delete_elem(int64_t payload_key) {
        Node *ref_ptr = back;

        for (int64_t i = 0; i < size; ++i) {
            if (ref_ptr->read_payload() == payload_key) {
                if (ref_ptr != front && ref_ptr != back) {
                    auto ref_ptr_prev_ref_val = reinterpret_cast<Node *>(ref_ptr->read_prev_ref())->read_payload();

                    reinterpret_cast<Node *>(ref_ptr->read_prev_ref())
                            ->write(&ref_ptr_prev_ref_val, ref_ptr->read_next_ref(),
                                    reinterpret_cast<Node *>(ref_ptr->read_prev_ref())->read_prev_ref());
                } else if (ref_ptr == back && ref_ptr != front) {
                    back = reinterpret_cast<Node *>(ref_ptr->read_next_ref());
                } else if (ref_ptr == front && ref_ptr != back) {
                    front = reinterpret_cast<Node *>(ref_ptr->read_prev_ref());
                } else if (ref_ptr == front && ref_ptr == back) {
                    front = nullptr;
                    back = nullptr;
                }

                delete ref_ptr;
                --size;
                return;
            }
            ref_ptr = reinterpret_cast<Node *>(ref_ptr->read_next_ref());
        }
    }

    auto getSize() { return size; }

    auto print() {
        Node *ref_ptr = front;
        std::vector<Node *> checkVec;

        for (int64_t i = 0; i < size; ++i) {
            //            std::cout << ref_ptr->read_payload() << "\n";
            ref_ptr = reinterpret_cast<Node *>(ref_ptr->read_prev_ref());

            checkVec.emplace_back(ref_ptr);
        }

        return checkVec;
    }

private:
    Node *front = nullptr;
    Node *back = nullptr;
    intptr_t size = 0;
};

class DCDS_DL_LinkedList {
public:
    void push_front(int64_t payload) {
        auto new_node_ptr = new DCDS_Node;
        new_node_ptr->write(&payload, nullptr, front);

        if (front != nullptr) {
            auto front_val = front->read_payload();

            front->write(&front_val, new_node_ptr, front->read_prev_ref());
        } else {
            back = new_node_ptr;
        }
        front = new_node_ptr;
        ++size;
    }

    auto pop_back() {
        if (back == nullptr) return static_cast<int64_t>(-1);

        int64_t return_val = back->read_payload();
        if (front != back) {
            back = reinterpret_cast<DCDS_Node *>(back->read_next_ref());
        } else {
            back = nullptr;
            front = nullptr;
        }

        --size;
        return return_val;
    }

    void delete_elem(int64_t payload_key) {
        DCDS_Node *ref_ptr = back;

        for (int64_t i = 0; i < size; ++i) {
            if (ref_ptr->read_payload() == payload_key) {
                if (ref_ptr != front && ref_ptr != back) {
                    auto ref_ptr_prev_ref_val = reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref())->read_payload();

                    reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref())
                            ->write(&ref_ptr_prev_ref_val, ref_ptr->read_next_ref(),
                                    reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref())->read_prev_ref());
                } else if (ref_ptr == back && ref_ptr != front) {
                    back = reinterpret_cast<DCDS_Node *>(ref_ptr->read_next_ref());
                } else if (ref_ptr == front && ref_ptr != back) {
                    front = reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref());
                } else if (ref_ptr == front && ref_ptr == back) {
                    front = nullptr;
                    back = nullptr;
                }

                delete ref_ptr;
                --size;
                return;
            }
            ref_ptr = reinterpret_cast<DCDS_Node *>(ref_ptr->read_next_ref());
        }
    }

    auto getSize() { return size; }

    auto print() {
        DCDS_Node *ref_ptr = front;
        std::vector<DCDS_Node *> checkVec;

        for (int64_t i = 0; i < size; ++i) {
            //        LOG(INFO) << ref_ptr->read_payload();
            ref_ptr = reinterpret_cast<DCDS_Node *>(ref_ptr->read_prev_ref());

            checkVec.emplace_back(ref_ptr);
        }
        return checkVec;
    }

private:
    DCDS_Node *front = nullptr;
    DCDS_Node *back = nullptr;
    intptr_t size = 0;
};

static void benchmark_dcds_dll(benchmark::State &state) {
    DCDS_DL_LinkedList dll;

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            dll.push_front(1);
            dll.push_front(2);
            dll.push_front(3);
            dll.push_front(1);
            dll.push_front(4);
            dll.push_front(5);
        }
    }
}

static void benchmark_normal_dll(benchmark::State &state) {
    DL_LinkedList dll;

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            dll.push_front(1);
            dll.push_front(2);
            dll.push_front(3);
            dll.push_front(1);
            dll.push_front(4);
            dll.push_front(5);
        }
    }
}

static void benchmark_dcds_dll_read(benchmark::State &state) {
    DCDS_DL_LinkedList dll;
    dll.push_front(1);
    dll.push_front(2);
    dll.push_front(3);
    dll.push_front(1);
    dll.push_front(4);
    dll.push_front(5);

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            dll.print();
        }
    }
}

static void benchmark_normal_dll_read(benchmark::State &state) {
    DL_LinkedList dll;
    dll.push_front(1);
    dll.push_front(2);
    dll.push_front(3);
    dll.push_front(1);
    dll.push_front(4);
    dll.push_front(5);

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            dll.print();
        }
    }
}

static void assertBothListsProduceEqualResults() {
    DCDS_DL_LinkedList dll_dcds;
    DL_LinkedList dll;

    for (uint64_t i = 0; i < 30; ++i) {
        auto query_num = std::rand() % 100;
        dll_dcds.push_front(query_num);
        dll.push_front(query_num);
    }

    auto read_dcds = dll_dcds.print();
    auto read_normal = dll.print();

    for (uint64_t i = 0; i < 4; ++i) {
        assert(read_dcds[i]->read_payload() == read_normal[i]->read_payload());
    }
}

static void benchmark_dcds_dll_pop_back(benchmark::State &state) {
    DCDS_DL_LinkedList dll;
    dll.push_front(1);
    dll.push_front(2);
    dll.push_front(3);
    dll.push_front(1);
    dll.push_front(4);
    dll.push_front(5);

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            for (int j = 0; j < 6; ++j)
                dll.pop_back();
        }
    }
}

static void benchmark_normal_dll_pop_back(benchmark::State &state) {
    DL_LinkedList dll;
    dll.push_front(1);
    dll.push_front(2);
    dll.push_front(3);
    dll.push_front(1);
    dll.push_front(4);
    dll.push_front(5);

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            for (int j = 0; j < 6; ++j)
                dll.pop_back();
        }
    }
}

static void benchmark_dcds_dll_delete_elem(benchmark::State &state) {
    DCDS_DL_LinkedList dll;
    dll.push_front(1);
    dll.push_front(2);
    dll.push_front(3);
    dll.push_front(1);
    dll.push_front(4);
    dll.push_front(5);

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            dll.delete_elem(1);
            dll.delete_elem(5);
            dll.delete_elem(4);
            dll.delete_elem(2);
            dll.delete_elem(1);
            dll.delete_elem(3);
        }
    }
}

static void benchmark_normal_dll_delete_elem(benchmark::State &state) {
    DL_LinkedList dll;
    dll.push_front(1);
    dll.push_front(2);
    dll.push_front(3);
    dll.push_front(1);
    dll.push_front(4);
    dll.push_front(5);

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            dll.delete_elem(1);
            dll.delete_elem(5);
            dll.delete_elem(4);
            dll.delete_elem(2);
            dll.delete_elem(1);
            dll.delete_elem(3);
        }
    }
}

int main(int argc, char **argv) {
    DCDS_Node::initialize();

    BENCHMARK(benchmark_dcds_dll)->Arg(1)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll)->Arg(1)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll)->Arg(4)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll)->Arg(4)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll)->Arg(8)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll)->Arg(8)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll)->Arg(16)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll)->Arg(16)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_read)->Arg(1)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_read)->Arg(1)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_read)->Arg(4)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_read)->Arg(4)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_read)->Arg(8)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_read)->Arg(8)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_read)->Arg(16)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_read)->Arg(16)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_pop_back)->Arg(1)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_pop_back)->Arg(1)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_pop_back)->Arg(4)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_pop_back)->Arg(4)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_pop_back)->Arg(8)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_pop_back)->Arg(8)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_pop_back)->Arg(16)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_pop_back)->Arg(16)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_delete_elem)->Arg(1)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_delete_elem)->Arg(1)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_delete_elem)->Arg(4)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_delete_elem)->Arg(4)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_delete_elem)->Arg(8)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_delete_elem)->Arg(8)->Unit(benchmark::kMicrosecond);

    BENCHMARK(benchmark_dcds_dll_delete_elem)->Arg(16)->Unit(benchmark::kMicrosecond);
    BENCHMARK(benchmark_normal_dll_delete_elem)->Arg(16)->Unit(benchmark::kMicrosecond);

    assertBothListsProduceEqualResults();

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
