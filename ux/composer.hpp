/// composer is the abstract for the manager of component instancing, propagation & rendition; 150  lines
struct Composer {
    App::Interface *ux;
    node      *root = null;
    
    bool process() {
        return root ? root->process() : true;
    }

    Composer(App::Interface *ux) : ux(ux) { }

    std::string element_id(Element &e) {
        bool has_id = e.args.count("id");
        char buf[128];
        if (!has_id)
            sprintf(buf, "%p", (void *)e.factory);
        return has_id ? std::string(e.args["id"]) : std::string(buf);
    }

    bool update(node *p, node **pn, Element &e) {
        node *n = *pn;
        bool node_updated = false;
        
        if (!n) {
            n         = (*pn = e.factory());
            n->root   = p ? p->root : n;
            if (!p) {
                /// root element is initialized with the window size
                n->path = rectd { 0.0, 0.0, double(ux->sz.x), double(ux->sz.y) };
            }
            n->parent = p;
            n->define_standard();
            n->define();
            node_updated = true;
        } else if (!e) {
            assert(false);
            if (n) {
                if (n->mounted)
                    n->umount();
                n->parent = null;
                delete n;
               *pn = null;
            }
            return true;
        }
        
        /// update props, and elements
        var d_null = nullptr;
        map<std::string, bool> dropped;
        for (auto &[prop_id, d_new]: n->args)
            dropped[prop_id] = true;
        
        struct PChange {
            enum {
                Undefined, Change, Drop
            } type;
            std::string prop_id;
        };
        auto props_changed = vec<PChange>(e.args.size());
        for (auto &[prop_id, d_new]: e.args) {
            bool exists = n->args.count(prop_id);
            var &d_cur = exists ? n->args[prop_id] : d_null; // let it grow in iteration-1
            if (d_cur != d_new) {
                /// prop data update on nodes
                n->args[prop_id] = e.args[prop_id]; // this pointer is changed because its reallocated.
                props_changed += { PChange::Change, prop_id }; // fixed: &d_new was used, and this data just exists in the element args which get erased, from existence.
            }
            if (dropped.count(prop_id))
                dropped[prop_id] = false;
        }
        
        /// drop unused props
        for (auto &[prop_id, d_new]: n->args) {
            if (dropped[prop_id]) {
                props_changed += { PChange::Drop, prop_id };
                n->args.erase(prop_id);
            }
        }
        
        /// check if the elements are different
        if (n->elements != e.elements) {
            n->elements  = e.elements;
            node_updated = true;
        }
        
        if (props_changed.size())
            node_updated = true;
        
        /// update props to default or given prop
        for (auto &p: props_changed) {
            assert(n->defs.count(p.prop_id));
            auto &def = n->defs[p.prop_id];
            def.setter(this, p.type == PChange::Change ? n->args[p.prop_id] : def.data);
        }
        
        auto u = map<std::string, bool>();
        for (auto &[id, nn]: n->mounts)
            u[id] = true;
        
        /// render uses elements data to produce the instances
        if (!n->mounted) {
             n->mount();
             n->mounted = true;
        }
        
        Element ee = n->render();
        if (ee.ref) {
            assert(ee.ref == n);
            node        *n = ee.ref;
            size_t      sz = n->elements.size();
            auto        um = map<std::string, bool>(sz);
            for (size_t  i = 0; i < n->elements.size(); i++) {
                auto  &eee = n->elements[i];
                std::string id = element_id(eee);
                update(n, &n->mounts[id], eee);
                u[id] = false;
            }
        } else {
            std::string id = element_id(ee);
            update(n, &n->mounts[id], ee);
            u[id] = false;
        }
        
        /// perform unmounting
        Element e_null = nullptr;
        for (auto &[id, unmount]: u)
            if (unmount) {
                std::string i = id;
                update(n, &n->mounts[id], e_null);
                node_updated = true;
            }
        
        return node_updated;
    }
    
    /// main update / render / draw function
    void operator()(Element base_render) {
        update(null, &root, base_render);
        root->select([&](node *n) {
            if (n != root)
                n->path = n->props.region(n, n->parent);
            return null;
        });
    }
    
    vec<node *> query(std::function<node *(node *)> fn) {
        return root->select(fn);
    }
    
    void draw() {
        ux->draw(root);
    }

    void input(str v) {
        if (root) {
            node *f = root->get_state("focus");
            (f ? f : root)->input(v);
        }
    }
};
