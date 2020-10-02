//
//  tree_builder.cxx
//  wtfhd
//
//  Created by Kirill Bystrov on 8/8/20.
//

#include "tree_builder.hxx"

#include <map>
#include <set>
#include <thread>
#include <cassert>

#include "misc_types.hxx"
#include "integral_set.hxx"

using namespace fs;
using namespace std;
using namespace util;

namespace fs::impl {
	struct node_id_set: public util::integral_set <::dev_t, ::ino_t> {
	private:
		typedef util::integral_set <dev_t, ino_t> super;
		
	public:
		typedef node_info::id node_id_t;
		
		node_id_set (): super () {}
		node_id_set (std::initializer_list <node_id_t> const &values): node_id_set (values.begin (), values.end ()) {}
		
		~node_id_set () = default;
		
		bool contains (node_id_t const &value) const {
			return std::apply (&super::contains, std::tuple_cat (std::make_tuple (this), value.as_tuple ()));
		}
		
		bool insert (node_id_t const &value) {
			return std::apply (&super::insert, std::tuple_cat (std::make_tuple (this), value.as_tuple ()));
		}
		
		bool remove (node_id_t const &value) {
			return std::apply (&super::remove, std::tuple_cat (std::make_tuple (this), value.as_tuple ()));
		}
		
	private:
		template <typename _It>
		struct node_id_iterator_wrapper {
		public:
			node_id_iterator_wrapper (_It const &other): _impl (other) {}
			
			auto operator* () {
				return this->_impl->as_tuple ();
			}
			
			bool operator== (node_id_iterator_wrapper const &other) const {
				return this->_impl == other._impl;
			}
			
			node_id_iterator_wrapper &operator++ (int) {
				this->_impl++;
				return *this;
			}

		private:
			_It _impl;
		};
		
		template <typename _It>
		node_id_set (_It values_begin, _It values_end): super (node_id_iterator_wrapper (values_begin), node_id_iterator_wrapper (values_end)) {}
	};
	
	class tree_builder: public ::tree_builder {
	public:
		tree_builder (unique_ptr <children_policy const> &&policy): _policy (std::move (policy)) {
			
		}
		
		virtual bool started () const override {
			return !!this->_completion_callback;
		}
		
		virtual progress_t progress () const override {
			return progress_t { this->_ready, this->_total };
		}
		
		virtual bool ready () const override {
			return this->_result.has_value ();
		}
		
		virtual bool success () const override {
			return this->_result.value ();
		}
		
		virtual std::optional <bool> result () const override {
			return this->_result;
		}
		
		virtual bool contains (node_id_t const &node_id) const override;
		virtual void add_node (node_id_t const &node_id) override;
		
		virtual callback_id_t add_progress_callback (callback_t const &callback) override;
		virtual void remove_progress_callback (callback_id_t const callback_id) override;
		
		virtual void start (callback_t const &callback) override;
		virtual void cancel () override;

	private:
		struct callback_before {
			static callback_id_t identifiier (callback_t const &callback) {
				return reinterpret_cast <callback_id_t> (callback.target <void ()> ());
			}

			static callback_t callback (callback_id_t const &id) {
				return reinterpret_cast <void (*) ()> (id);
			}
			
			bool operator () (callback_t const &lhs, callback_t const &rhs) const {
				return identifiier (lhs) < identifiier (rhs);
			}
		};
		
		void run ();
		
		unique_ptr <children_policy const> const _policy;
		callback_t _completion_callback;
		set <callback_t, callback_before> _progress_callbacks;
				
		size_t _ready;
		size_t _total;
		tristate_bool _result;

		node_id_set _pending, _processed;
	};
}

unique_ptr <tree_builder> tree_builder::make_unique (unique_ptr <children_policy const> &&policy) {
	return make_unique <impl::tree_builder> (forward <unique_ptr <children_policy const>> (policy));
}

bool impl::tree_builder::contains (node_id_t const &node_id) const {
	return this->_processed.contains (node_id);
}

void impl::tree_builder::add_node (node_id_t const &node_id) {
	assert (!this->started ());
	this->_pending.insert (node_id);
	this->_total++;
}

callback_id_t impl::tree_builder::add_progress_callback (callback_t const &callback) {
	assert (!this->started ());
	this->_progress_callbacks.insert (callback);
	return callback_before::identifiier (callback);
}

void impl::tree_builder::remove_progress_callback (callback_id_t const callback_id) {
	this->_progress_callbacks.erase (callback_before::callback (callback_id));
}

void impl::tree_builder::start (callback_t const &callback) {
	this->_completion_callback = callback;
	thread (&tree_builder::run, this).detach ();
}

void impl::tree_builder::cancel () {
	assert (!this->ready ());
	this->_result = false;
}

void impl::tree_builder::run () {
	
}
