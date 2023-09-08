//
// Created by prathamesh on 28/8/23.
//

#ifndef DCDS_NODE_HPP
#define DCDS_NODE_HPP

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

    auto write_payload(int64_t *payload) { built_write_payload_fn(storageObjectPtr, payload); }
    auto write_next_ref(void *next_ref) { built_write_next_ref_fn(storageObjectPtr, next_ref); }
    auto write_prev_ref(void *prev_ref) { built_write_prev_ref_fn(storageObjectPtr, prev_ref); }

private:
    static void (*built_write_payload_fn)(void *, int64_t *);
    static void (*built_write_next_ref_fn)(void *, void *);
    static void (*built_write_prev_ref_fn)(void *, void *);
    static void *(*built_read_prev_ref_fn)(void *);
    static void *(*built_read_next_ref_fn)(void *);
    static int64_t (*built_read_payload_fn)(void *);
    static std::shared_ptr<dcds::Visitor> visitor;

public:
    std::shared_ptr<dcds::StorageLayer> storageObject;
    void *storageObjectPtr;
};

void (*DCDS_Node::built_write_payload_fn)(void *, int64_t *);
void (*DCDS_Node::built_write_next_ref_fn)(void *, void *);
void (*DCDS_Node::built_write_prev_ref_fn)(void *, void *);
void *(*DCDS_Node::built_read_prev_ref_fn)(void *);
void *(*DCDS_Node::built_read_next_ref_fn)(void *);
int64_t (*DCDS_Node::built_read_payload_fn)(void *);
std::shared_ptr<dcds::Visitor> DCDS_Node::visitor;

#endif //DCDS_NODE_HPP
