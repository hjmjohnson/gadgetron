#include "GrappaWeightsCalculator.h"
#include "GadgetContainerMessage.h"

#include "Gadgetron.h"
#include "b1_map.h"
#include "hoNDArray_fileio.h"
#include "cuNDFFT.h"
#include "htgrappa.h"
#include "GPUTimer.h"

template <class T> class GrappaWeightsDescription
{

public:
  std::vector< std::pair<unsigned int, unsigned int> > sampled_region;
  unsigned int acceleration_factor;
  GrappaWeights<T>* destination;
  std::vector<unsigned int> uncombined_channel_weights;
  bool include_uncombined_channels_in_combined_weights;
};

template <class T> int GrappaWeightsCalculator<T>::svc(void) 
{
   ACE_TRACE(( ACE_TEXT("GrappaWeightsCalculator::svc") ));

   ACE_Message_Block *mb;
    
   while (this->getq(mb) >= 0) {   
     if (mb->msg_type() == ACE_Message_Block::MB_HANGUP) {
       if (this->putq(mb) == -1) {
	 ACE_ERROR_RETURN( (LM_ERROR,
			    ACE_TEXT("%p\n"),
			    ACE_TEXT("GrappaWeightsCalculator::svc, putq")),
			   -1);
       }
       break;
     }

     GadgetContainerMessage< GrappaWeightsDescription<T> >* mb1 
       = AsContainerMessage< GrappaWeightsDescription<T> >(mb);

     if (!mb1) {
       mb->release();
       return -2;
     }

     GadgetContainerMessage< hoNDArray< std::complex<T> > >* mb2 
       = AsContainerMessage< hoNDArray< std::complex<T> > >(mb1->cont());

     if (!mb2) {
       mb->release();
       return -3;
     }

     hoNDArray<float_complext::Type>* host_data = 
       reinterpret_cast< hoNDArray<float_complext::Type>* >(mb2->getObjectPtr());


     // Copy the image data to the device
     cuNDArray<float_complext::Type> device_data(*host_data);
     device_data.squeeze();
     
     std::vector<unsigned int> ftdims(2,0); ftdims[1] = 1;
     cuNDFFT ft;

     //Go to image space
     ft.ifft(reinterpret_cast< cuNDArray<cuFloatComplex>* >(&device_data),ftdims);

     // Compute CSM
     auto_ptr< cuNDArray<float_complext::Type> > csm = estimate_b1_map<float,2>( &device_data );

     //Go back to kspace
     ft.fft(reinterpret_cast< cuNDArray<cuFloatComplex>* >(&device_data),ftdims);


     //TODO: Change dimensions of this to deal with uncombinex channels
     cuNDArray<cuFloatComplex> unmixing_dev;
     if (!unmixing_dev.create(csm.get()->get_dimensions())) {
       GADGET_DEBUG1("Unable to allocate device memory for unmixing coeffcients\n");
       return GADGET_FAIL;
     }

     {
       //GPUTimer unmix_timer("GRAPPA Unmixing");
       std::vector<unsigned int> kernel_size;

       //TODO: Add parameters for kernel size
       kernel_size.push_back(5);
       kernel_size.push_back(4);
       if ( htgrappa_calculate_grappa_unmixing(reinterpret_cast< cuNDArray<cuFloatComplex>* >(&device_data), 
					       reinterpret_cast< cuNDArray<cuFloatComplex>* >(csm.get()),
					       mb1->getObjectPtr()->acceleration_factor,
					       kernel_size,
					       &unmixing_dev) < 0) {
	 GADGET_DEBUG1("GRAPPA unmixing coefficients calculation failed\n");
	 return GADGET_FAIL;
       }
     }

     if (mb1->getObjectPtr()->destination) {
       hoNDArray<cuFloatComplex> unmixing_host = unmixing_dev.to_host();

       //TODO: This reshaping needs to take uncombined channels into account
       if (unmixing_host.reshape(mb2->getObjectPtr()->get_dimensions()) < 0) {
	 GADGET_DEBUG1("Failed to reshape GRAPPA weights\n");
	 return GADGET_FAIL;
       }

       if (mb1->getObjectPtr()->destination->update(reinterpret_cast<hoNDArray<std::complex<float> >* >(&unmixing_host)) < 0) {
	 GADGET_DEBUG1("Update of GRAPPA weights failed\n");
	 return GADGET_FAIL;
       }
     } else {
       GADGET_DEBUG1("Undefined GRAPPA weights destination\n");
       return GADGET_FAIL;
     }
     

     mb->release();
   }

   return 0;
}

template <class T> int GrappaWeightsCalculator<T>::close(unsigned long flags)
{
  ACE_TRACE(( ACE_TEXT("GrappaWeightsCalculator::close") ));
  
  int rval = 0;
  if (flags == 1) {
    ACE_Message_Block *hangup = new ACE_Message_Block();
    hangup->msg_type( ACE_Message_Block::MB_HANGUP );
    if (this->putq(hangup) == -1) {
	hangup->release();
	ACE_ERROR_RETURN( (LM_ERROR,
			   ACE_TEXT("%p\n"),
			   ACE_TEXT("GrappaWeightsCalculator::close, putq")),
			  -1);
    }
    rval = this->wait();
  }
  return rval;
}


template <class T> int GrappaWeightsCalculator<T>::
add_job( hoNDArray< std::complex<T> >* ref_data,
	 std::vector< std::pair<unsigned int, unsigned int> > sampled_region,
	 unsigned int acceleration_factor,
	 GrappaWeights<T>* destination,
	 std::vector<unsigned int> uncombined_channel_weights,
	 bool include_uncombined_channels_in_combined_weights)
{
   
  GadgetContainerMessage< GrappaWeightsDescription<T> >* mb1 = 
    new GadgetContainerMessage< GrappaWeightsDescription<T> >();

  if (!mb1) {
    return -1;
  }

  mb1->getObjectPtr()->sampled_region = sampled_region;
  mb1->getObjectPtr()->acceleration_factor = acceleration_factor;
  mb1->getObjectPtr()->destination = destination;
  mb1->getObjectPtr()->uncombined_channel_weights = uncombined_channel_weights;
  mb1->getObjectPtr()->include_uncombined_channels_in_combined_weights = 
    include_uncombined_channels_in_combined_weights;


  GadgetContainerMessage< hoNDArray< std::complex<T> > >* mb2 = 
    new GadgetContainerMessage< hoNDArray< std::complex<T> > >();

  if (!mb2) {
    mb1->release();
    return -2;
  }

  mb1->cont(mb2);

  if (!mb2->getObjectPtr()->create(ref_data->get_dimensions())) {
    mb1->release();
    return -3;
  }

  memcpy(mb2->getObjectPtr()->get_data_ptr(), ref_data->get_data_ptr(),
	 ref_data->get_number_of_elements()*sizeof(T)*2);
  
  this->putq(mb1);

  return 0;
}

template class GrappaWeightsCalculator<float>;
