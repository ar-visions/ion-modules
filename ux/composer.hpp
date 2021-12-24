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
                str   p_id    = p->m.id;
                str   p_cn    = p->class_name;
                auto &dst     = (node::Member &)(*n->externals[bind.id]);
                bool  has_int = p->internals.count(bind.to) > 0;
                bool  has_ext = p->externals.count(bind.to) > 0;
                if (!has_int && !has_ext)
                    console.fatal("bind:{0} not found on node:{1}", {bind.to, p_cn, p_id});
                auto &src     = (node::Member &)(has_int ? *p->internals[bind.to] : *p->externals[bind.to]);
                if (dst != src) {
                    changed  += bind.id;
                    dst       = src;
                }
            } else {
                str n_id = n->m.id;
                str n_cn = n->class_name;
                console.fatal("external:{0} not found on {1}:{2}", {bind.id, n_cn, n_id});
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
        } else if (ee.factory) {
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
        if (node_updated)
            n->changed(changed);
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
