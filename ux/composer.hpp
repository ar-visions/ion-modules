/// composer is the abstract for managing component instancing, propagation & rendition
struct Composer {
    App::Interface *ux;
    node *root = null;
    Args args;
    /// args need to make their way to vulkan so the window can be created in any default size
    
    bool process() {
        return root ? root->process() : true;
    }

    Composer(App::Interface *ux, Args &args) : ux(ux), args(args) { }

    std::string element_id(Element &e) {
        str id;
        for (auto &bind: e.binds)
            if (bind.id == "id") {
                id = bind.to;
                break;
            }
        char buf[128];
        if (!id)
            sprintf(buf, "%p", (void *)e.factory);
        return id ? std::string(id) : std::string(buf);
    }

    bool update(node *p, node **pn, Element &e) {
        node *n = *pn;
        bool node_updated = false;
        ///
        if (!n) {
            n         = (*pn = e.factory());
            n->root   = p ? p->root : n;
            if (!p) {
                /// root element is initialized with the window size
                n->path = rectd { 0.0, 0.0, double(ux->sz.x), double(ux->sz.y) };
            }
            n->parent = p;
            n->standard();
            n->bind();
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
        for (auto &bind: n->binds)
            dropped[bind.id] = true;
        ///
        auto changed = vec<str>(e.binds.size());
        for (auto &bind: e.binds) {
            bool exists = n->externals.count(bind.id); // assertion
            if (exists) {
                node::Member &dst = *n->externals[bind.id];
                if (p->binds.count(bind.to) == 0)
                    console.fatal("bind-to:{0} not found on node:{1}", {bind.to, str(n->class_name)});
                node::Member &src = *p->internals[bind.to];
                if (dst != src) {
                    changed += bind.id;
                    dst      = src;
                }
            } else {
                console.fatal("bind-from:{0} not found on node:{1}", {bind.id, str(n->class_name)});
            }

            if (dropped.count(bind.id))
                dropped[bind.id] = false;
        }
        ///
        /// drop unused props
        bool cycle;
        do {
            size_t index = 0;
            cycle        = false;
            for (auto &bind: n->binds) {
                if (dropped[bind.id]) {
                    changed += bind.id;
                    n->externals[bind.id]->unset();
                    n->binds.erase(index);
                    cycle = true;
                    break;
                }
                index++;
            }
        } while (cycle);
        ///
        /// check if the elements are different
        if (n->elements != e.elements) {
            n->elements  = e.elements;
            node_updated = true;
        }
        ///
        if (changed.size())
            node_updated = true;
        
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
            if (n != root) {
                Region &region = n->m.region;
                n->path = region(n, n->parent);
            }
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
            node *f = root ->focused();
            (f  ? f : root)->input(v);
        }
    }
};
