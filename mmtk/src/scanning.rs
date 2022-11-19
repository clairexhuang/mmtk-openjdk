use super::gc_work::*;
use super::{NewBuffer, OpenJDKEdge, SINGLETON, UPCALLS};
use crate::{EdgesClosure, OpenJDK};
use mmtk::memory_manager;
use mmtk::scheduler::WorkBucketStage;
use mmtk::util::opaque_pointer::*;
use mmtk::util::{Address, ObjectReference};
use mmtk::vm::{EdgeVisitor, RootsWorkFactory, Scanning};
use mmtk::Mutator;
use mmtk::MutatorContext;
use probe::probe;

pub struct VMScanning {}

const WORK_PACKET_CAPACITY: usize = 4096;

extern "C" fn report_edges_and_renew_buffer<F: RootsWorkFactory<OpenJDKEdge>>(
    ptr: *mut Address,
    length: usize,
    capacity: usize,
    factory_ptr: *mut F,
) -> NewBuffer {
    if !ptr.is_null() {
        let buf = unsafe { Vec::<Address>::from_raw_parts(ptr, length, capacity) };
        let factory: &mut F = unsafe { &mut *factory_ptr };
        factory.create_process_edge_roots_work(buf);
    }
    let (ptr, _, capacity) = {
        // TODO: Use Vec::into_raw_parts() when the method is available.
        use std::mem::ManuallyDrop;
        let new_vec = Vec::with_capacity(WORK_PACKET_CAPACITY);
        let mut me = ManuallyDrop::new(new_vec);
        (me.as_mut_ptr(), me.len(), me.capacity())
    };
    NewBuffer { ptr, capacity }
}

pub(crate) fn to_edges_closure<F: RootsWorkFactory<OpenJDKEdge>>(factory: &mut F) -> EdgesClosure {
    EdgesClosure {
        func: report_edges_and_renew_buffer::<F> as *const _,
        data: factory as *mut F as *mut libc::c_void,
    }
}

// store edges for every object 
struct SaveEdges {
    edges: Vec<OpenJDKEdge>
}

impl EdgeVisitor<OpenJDKEdge> for SaveEdges {
    fn visit_edge(&mut self, edge: OpenJDKEdge) {
        self.edges.push(edge)
    }
}

impl SaveEdges {
    fn new() -> Self {
        // return save edges instance of empty vector 
        SaveEdges { edges: vec![] }
    }
}
impl Scanning<OpenJDK> for VMScanning {
    const SCAN_MUTATORS_IN_SAFEPOINT: bool = false;
    const SINGLE_THREAD_MUTATOR_SCANNING: bool = false;

    fn scan_object<EV: EdgeVisitor<OpenJDKEdge>>(
        tls: VMWorkerThread,
        object: ObjectReference,
        edge_visitor: &mut EV,
    ) {
        let mut save_edges = SaveEdges::new();
        crate::object_scanning::scan_object(object, &mut save_edges, tls);
        probe!(mmtk, scan_object, save_edges.edges.len());
        // println!("scan object {}", save_edges.edges.len());
        for edge in save_edges.edges {
            edge_visitor.visit_edge(edge);
        }
    }

    fn notify_initial_thread_scan_complete(_partial_scan: bool, _tls: VMWorkerThread) {
        // unimplemented!()
        // TODO
    }

    fn scan_thread_roots(_tls: VMWorkerThread, mut factory: impl RootsWorkFactory<OpenJDKEdge>) {
        unsafe {
            ((*UPCALLS).scan_all_thread_roots)(to_edges_closure(&mut factory));
        }
    }

    fn scan_thread_root(
        _tls: VMWorkerThread,
        mutator: &'static mut Mutator<OpenJDK>,
        mut factory: impl RootsWorkFactory<OpenJDKEdge>,
    ) {
        let tls = mutator.get_tls();
        unsafe {
            ((*UPCALLS).scan_thread_roots)(to_edges_closure(&mut factory), tls);
        }
    }

    fn scan_vm_specific_roots(_tls: VMWorkerThread, factory: impl RootsWorkFactory<OpenJDKEdge>) {
        memory_manager::add_work_packets(
            &SINGLETON,
            WorkBucketStage::Prepare,
            vec![
                Box::new(ScanUniverseRoots::new(factory.clone())) as _,
                Box::new(ScanJNIHandlesRoots::new(factory.clone())) as _,
                Box::new(ScanObjectSynchronizerRoots::new(factory.clone())) as _,
                Box::new(ScanManagementRoots::new(factory.clone())) as _,
                Box::new(ScanJvmtiExportRoots::new(factory.clone())) as _,
                Box::new(ScanAOTLoaderRoots::new(factory.clone())) as _,
                Box::new(ScanSystemDictionaryRoots::new(factory.clone())) as _,
                Box::new(ScanCodeCacheRoots::new(factory.clone())) as _,
                Box::new(ScanStringTableRoots::new(factory.clone())) as _,
                Box::new(ScanClassLoaderDataGraphRoots::new(factory.clone())) as _,
                Box::new(ScanWeakProcessorRoots::new(factory.clone())) as _,
            ],
        );
        if !(Self::SCAN_MUTATORS_IN_SAFEPOINT && Self::SINGLE_THREAD_MUTATOR_SCANNING) {
            memory_manager::add_work_packet(
                &SINGLETON,
                WorkBucketStage::Prepare,
                ScanVMThreadRoots::new(factory),
            );
        }
    }

    fn supports_return_barrier() -> bool {
        unimplemented!()
    }

    fn prepare_for_roots_re_scanning() {
        unsafe {
            ((*UPCALLS).prepare_for_roots_re_scanning)();
        }
    }
}
