package be.dabradio.app

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import kotlinx.android.synthetic.main.single_service_layout.view.*

class ServiceListAdapter(val serviceList : ArrayList<Service>) : RecyclerView.Adapter<ServiceListAdapter.ServiceListViewHolder>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ServiceListViewHolder {
        val view = LayoutInflater.from(parent.context).inflate(R.layout.single_service_layout, parent, false)
        return ServiceListViewHolder(view)
    }

    override fun getItemCount(): Int {
        return serviceList.size
    }

    override fun onBindViewHolder(holder: ServiceListViewHolder, position: Int) {
        holder.bind(serviceList[position])
    }

    class ServiceListViewHolder(private val view : View) : RecyclerView.ViewHolder(view) {
        fun bind(service : Service) {
            view.serviceName.text = service.name
            view.setOnClickListener {
                service.setActive()
            }
        }
    }
}